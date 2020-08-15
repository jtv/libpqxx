/** Implementation of the pqxx::connection class.
 *
 * pqxx::connection encapsulates a connection to a database.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>

// For WSAPoll():
#if __has_include(<winsock2.h>)
#  include <winsock2.h>
#endif
#if __has_include(<ws2tcpip.h>)
#  include <ws2tcpip.h>
#endif
#if __has_include(<mstcpip.h>)
#  include <mstcpip.h>
#endif

// For poll():
#if __has_include(<poll.h>)
#  include <poll.h>
#endif

// For select() on recent POSIX systems.
#if __has_include(<sys/select.h>)
#  include <sys/select.h>
#endif

// For select() on some older POSIX systems.
#if __has_include(<sys/types.h>)
#  include <sys/types.h>
#endif
#if __has_include(<unistd.h>)
#  include <unistd.h>
#endif
#if __has_include(<sys/time.h>)
#  include <sys/time.h>
#endif


extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/binarystring"
#include "pqxx/config-internal-libpq.h"
#include "pqxx/nontransaction"
#include "pqxx/notification"
#include "pqxx/pipeline"
#include "pqxx/result"
#include "pqxx/strconv"
#include "pqxx/transaction"

#include "pqxx/internal/gates/errorhandler-connection.hxx"
#include "pqxx/internal/gates/result-connection.hxx"
#include "pqxx/internal/gates/result-creation.hxx"


extern "C"
{
  // The PQnoticeProcessor that receives an error or warning from libpq and
  // sends it to the appropriate connection for processing.
  void pqxx_notice_processor(void *conn, char const *msg) noexcept
  {
    reinterpret_cast<pqxx::connection *>(conn)->process_notice(msg);
  }


  // There's no way in libpq to disable a connection's notice processor.  So,
  // set an inert one to get the same effect.
  void inert_notice_processor(void *, char const *) noexcept {}
} // extern "C"


std::string pqxx::encrypt_password(char const user[], char const password[])
{
  std::unique_ptr<char, std::function<void(char *)>> p{
    PQencryptPassword(password, user), PQfreemem};
  return std::string{p.get()};
}


pqxx::connection::connection(connection &&rhs) :
        m_conn{rhs.m_conn}, m_unique_id{rhs.m_unique_id}
{
  rhs.check_movable();
  rhs.m_conn = nullptr;
}


void pqxx::connection::complete_init()
{
  if (m_conn == nullptr)
    throw std::bad_alloc{};
  try
  {
    if (PQstatus(m_conn) != CONNECTION_OK)
      throw broken_connection{PQerrorMessage(m_conn)};

    set_up_state();
  }
  catch (std::exception const &)
  {
    PQfinish(m_conn);
    throw;
  }
}


void pqxx::connection::init(char const options[])
{
  m_conn = PQconnectdb(options);
  complete_init();
}


void pqxx::connection::init(char const *params[], char const *values[])
{
  m_conn = PQconnectdbParams(params, values, 0);
  complete_init();
}


void pqxx::connection::check_movable() const
{
  if (m_trans.get() != nullptr)
    throw pqxx::usage_error{"Moving a connection with a transaction open."};
  if (not m_errorhandlers.empty())
    throw pqxx::usage_error{
      "Moving a connection with error handlers registered."};
  if (not m_receivers.empty())
    throw pqxx::usage_error{
      "Moving a connection with notification receivers registered."};
}


void pqxx::connection::check_overwritable() const
{
  if (m_trans.get() != nullptr)
    throw pqxx::usage_error{
      "Moving a connection onto one with a transaction open."};
  if (not m_errorhandlers.empty())
    throw pqxx::usage_error{
      "Moving a connection onto one with error handlers registered."};
  if (not m_receivers.empty())
    throw usage_error{
      "Moving a connection onto one "
      "with notification receivers registered."};
}


pqxx::connection &pqxx::connection::operator=(connection &&rhs)
{
  check_overwritable();
  rhs.check_movable();

  close();

  m_conn = rhs.m_conn;
  m_unique_id = rhs.m_unique_id;

  rhs.m_conn = nullptr;

  return *this;
}


pqxx::result pqxx::connection::make_result(
  internal::pq::PGresult *pgr, std::shared_ptr<std::string> const &query)
{
  if (pgr == nullptr)
  {
    if (is_open())
      throw failure(err_msg());
    else
      throw broken_connection{"Lost connection to the database server."};
  }
  auto const r{pqxx::internal::gate::result_creation::create(
    pgr, query, internal::enc_group(encoding_id()))};
  pqxx::internal::gate::result_creation{r}.check_status();
  return r;
}


int pqxx::connection::backendpid() const noexcept
{
  return (m_conn == nullptr) ? 0 : PQbackendPID(m_conn);
}


namespace
{
PQXX_PURE int socket_of(::pqxx::internal::pq::PGconn const *c) noexcept
{
  return (c == nullptr) ? -1 : PQsocket(c);
}
} // namespace


int pqxx::connection::sock() const noexcept
{
  return socket_of(m_conn);
}


int pqxx::connection::protocol_version() const noexcept
{
  return (m_conn == nullptr) ? 0 : PQprotocolVersion(m_conn);
}


int pqxx::connection::server_version() const noexcept
{
  return PQserverVersion(m_conn);
}


void pqxx::connection::set_variable(
  std::string_view var, std::string_view value)
{
  std::string cmd{"SET "};
  cmd.reserve(cmd.size() + var.size() + 1 + value.size());
  cmd.append(var);
  cmd.push_back('=');
  cmd.append(value);
  exec(cmd.c_str());
}


std::string pqxx::connection::get_variable(std::string_view var)
{
  std::string cmd{"SHOW "};
  cmd.append(var);
  return exec(cmd.c_str()).at(0).at(0).as(std::string{});
}


/** Set up various parts of logical connection state that may need to be
 * recovered because the physical connection to the database was lost and is
 * being reset, or that may not have been initialized yet.
 */
void pqxx::connection::set_up_state()
{
  if (auto const proto_ver{protocol_version()}; proto_ver < 3)
  {
    if (proto_ver == 0)
      throw broken_connection{"No connection."};
    else
      throw feature_not_supported{
        "Unsupported frontend/backend protocol version; 3.0 is the minimum."};
  }

  if (server_version() <= 90000)
    throw feature_not_supported{
      "Unsupported server version; 9.0 is the minimum."};

  // The default notice processor in libpq writes to stderr.  Ours does
  // nothing.
  // If the caller registers an error handler, this gets replaced with an
  // error handler that walks down the connection's chain of handlers.  We
  // don't do that by default because there's a danger: libpq may call the
  // notice processor via a result object, even after the connection has been
  // destroyed and the handlers list no longer exists.
  PQsetNoticeProcessor(m_conn, inert_notice_processor, nullptr);
}


bool pqxx::connection::is_open() const noexcept
{
  return status() == CONNECTION_OK;
}


void pqxx::connection::process_notice_raw(char const msg[]) noexcept
{
  if ((msg == nullptr) or (*msg == '\0'))
    return;
  auto const rbegin = m_errorhandlers.crbegin(),
             rend = m_errorhandlers.crend();
  for (auto i{rbegin}; (i != rend) and (**i)(msg); ++i)
    ;
}


void pqxx::connection::process_notice(char const msg[]) noexcept
{
  if (msg == nullptr)
    return;
  zview const view{msg};
  if (view.empty())
    return;
  else if (msg[view.size() - 1] == '\n')
    process_notice_raw(msg);
  else
    // Newline is missing.  Let the zview version of the code add it.
    process_notice(view);
}


void pqxx::connection::process_notice(zview msg) noexcept
{
  if (msg.empty())
    return;
  else if (msg[msg.size() - 1] == '\n')
    process_notice_raw(msg.c_str());
  else
    try
    {
      // Add newline.
      std::string buf;
      buf.reserve(msg.size() + 1);
      buf.assign(msg);
      buf.push_back('\n');
      process_notice_raw(buf.c_str());
    }
    catch (std::exception const &)
    {
      // If nothing else works, try writing the message without the newline.
      process_notice_raw(msg.c_str());
    }
}


void pqxx::connection::trace(FILE *out) noexcept
{
  if (m_conn)
  {
    if (out)
      PQtrace(m_conn, out);
    else
      PQuntrace(m_conn);
  }
}


void pqxx::connection::add_receiver(pqxx::notification_receiver *n)
{
  if (n == nullptr)
    throw argument_error{"Null receiver registered"};

  // Add to receiver list and attempt to start listening.
  auto const p{m_receivers.find(n->channel())};
  auto const new_value{receiver_list::value_type{n->channel(), n}};

  if (p == m_receivers.end())
  {
    // Not listening on this event yet, start doing so.
    auto const lq{
      std::make_shared<std::string>("LISTEN " + quote_name(n->channel()))};
    make_result(PQexec(m_conn, lq->c_str()), lq);
    m_receivers.insert(new_value);
  }
  else
  {
    m_receivers.insert(p, new_value);
  }
}


void pqxx::connection::remove_receiver(pqxx::notification_receiver *T) noexcept
{
  if (T == nullptr)
    return;

  try
  {
    auto needle{
      std::pair<std::string const, notification_receiver *>{T->channel(), T}};
    auto R{m_receivers.equal_range(needle.first)};
    auto i{find(R.first, R.second, needle)};

    if (i == R.second)
    {
      process_notice(
        "Attempt to remove unknown receiver '" + needle.first + "'");
    }
    else
    {
      // Erase first; otherwise a notification for the same receiver may yet
      // come in and wreak havoc.  Thanks Dragan Milenkovic.
      bool const gone{R.second == ++R.first};
      m_receivers.erase(i);
      if (gone)
        exec(("UNLISTEN " + quote_name(needle.first)).c_str());
    }
  }
  catch (std::exception const &e)
  {
    process_notice(e.what());
  }
}


bool pqxx::connection::consume_input() noexcept
{
  return PQconsumeInput(m_conn) != 0;
}


bool pqxx::connection::is_busy() const noexcept
{
  return PQisBusy(m_conn) != 0;
}


void pqxx::connection::cancel_query()
{
  using pointer = std::unique_ptr<PGcancel, std::function<void(PGcancel *)>>;
  constexpr int buf_size{500};
  std::array<char, buf_size> errbuf;
  pointer cancel{PQgetCancel(m_conn), PQfreeCancel};
  if (cancel == nullptr)
    throw std::bad_alloc{};

  auto const c{PQcancel(cancel.get(), errbuf.data(), buf_size)};
  if (c == 0)
    throw pqxx::sql_error{
      std::string{errbuf.data(), errbuf.size()}, "[cancel]"};
}


void pqxx::connection::set_verbosity(error_verbosity verbosity) noexcept
{
  PQsetErrorVerbosity(m_conn, static_cast<PGVerbosity>(verbosity));
}


namespace
{
/// Unique pointer to PGnotify.
using notify_ptr = std::unique_ptr<PGnotify, std::function<void(PGnotify *)>>;


/// Get one notification from a connection, or null.
notify_ptr get_notif(pqxx::internal::pq::PGconn *conn)
{
  return notify_ptr(PQnotifies(conn), PQfreemem);
}
} // namespace


int pqxx::connection::get_notifs()
{
  if (not consume_input())
    throw broken_connection{"Connection lost."};

  // Even if somehow we receive notifications during our transaction, don't
  // deliver them.
  if (m_trans.get() != nullptr)
    return 0;

  int notifs = 0;
  for (auto N{get_notif(m_conn)}; N.get(); N = get_notif(m_conn))
  {
    notifs++;

    auto const Hit{m_receivers.equal_range(std::string{N->relname})};
    for (auto i{Hit.first}; i != Hit.second; ++i) try
      {
        (*i->second)(N->extra, N->be_pid);
      }
      catch (std::exception const &e)
      {
        try
        {
          process_notice(
            "Exception in notification receiver '" + i->first +
            "': " + e.what() + "\n");
        }
        catch (std::bad_alloc const &)
        {
          // Out of memory.  Try to get the message out in a more robust way.
          process_notice(
            "Exception in notification receiver, "
            "and also ran out of memory\n");
        }
        catch (std::exception const &)
        {
          process_notice(
            "Exception in notification receiver "
            "(compounded by other error)\n");
        }
      }

    N.reset();
  }
  return notifs;
}


char const *pqxx::connection::dbname() const
{
  return PQdb(m_conn);
}


char const *pqxx::connection::username() const
{
  return PQuser(m_conn);
}


char const *pqxx::connection::hostname() const
{
  return PQhost(m_conn);
}


char const *pqxx::connection::port() const
{
  return PQport(m_conn);
}


char const *pqxx::connection::err_msg() const noexcept
{
  return (m_conn == nullptr) ? "No connection to database" :
                               PQerrorMessage(m_conn);
}


void pqxx::connection::register_errorhandler(errorhandler *handler)
{
  // Set notice processor on demand, i.e. only when the caller actually
  // registers an error handler.
  // We do this just to make it less likely that users fall into the trap
  // where a result object may hold a notice processor derived from its parent
  // connection which has already been destroyed.  Our notice processor goes
  // through the connection's list of error handlers.  If the connection object
  // has already been destroyed though, that list no longer exists.
  // By setting the notice processor on demand, we absolve users who never
  // register an error handler from ahving to care about this nasty subtlety.
  if (m_errorhandlers.empty())
    PQsetNoticeProcessor(m_conn, pqxx_notice_processor, this);
  m_errorhandlers.push_back(handler);
}


void pqxx::connection::unregister_errorhandler(errorhandler *handler) noexcept
{
  // The errorhandler itself will take care of nulling its pointer to this
  // connection.
  m_errorhandlers.remove(handler);
  if (m_errorhandlers.empty())
    PQsetNoticeProcessor(m_conn, inert_notice_processor, nullptr);
}


std::vector<pqxx::errorhandler *> pqxx::connection::get_errorhandlers() const
{
  return std::vector<errorhandler *>{
    std::begin(m_errorhandlers), std::end(m_errorhandlers)};
}


pqxx::result pqxx::connection::exec(std::string_view query)
{
  return exec(std::make_shared<std::string>(query));
}


pqxx::result pqxx::connection::exec(std::shared_ptr<std::string> query)
{
  auto const res{make_result(PQexec(m_conn, query->c_str()), query)};
  get_notifs();
  return res;
}


std::string pqxx::connection::encrypt_password(
  char const user[], char const password[], char const *algorithm)
{
#if defined(PQXX_HAVE_PQENCRYPTPASSWORDCONN)
  {
    auto const buf{PQencryptPasswordConn(m_conn, password, user, algorithm)};
    std::unique_ptr<char const, std::function<void(char const *)>> ptr{
      buf, [](char const *x) { PQfreemem(const_cast<char *>(x)); }};
    return std::string(ptr.get());
  }
#else
  {
    // No PQencryptPasswordConn.  Fall back on the old PQencryptPassword...
    // unless the caller selects a different algorithm.
    if (algorithm != nullptr and std::strcmp(algorithm, "md5") != 0)
      throw feature_not_supported{
        "Could not encrypt password: available libpq version does not support "
        "algorithms other than md5."};
    return pqxx::encrypt_password(user, password);
  }
#endif // PQXX_HAVE_PQENCRYPTPASSWORDCONN
}


void pqxx::connection::prepare(char const name[], char const definition[])
{
  // Allocate once, re-use across invocations.
  static auto const q{std::make_shared<std::string>("[PREPARE]")};

  auto const r{
    make_result(PQprepare(m_conn, name, definition, 0, nullptr), q)};
}


void pqxx::connection::prepare(char const definition[])
{
  this->prepare("", definition);
}


void pqxx::connection::unprepare(std::string_view name)
{
  exec("DEALLOCATE " + quote_name(name));
}


pqxx::result pqxx::connection::exec_prepared(
  std::string_view statement, internal::params const &args)
{
  auto const pointers{args.get_pointers()};
  auto const q{std::make_shared<std::string>(statement)};
  auto const pq_result{PQexecPrepared(
    m_conn, q->c_str(), check_cast<int>(args.nonnulls.size(), "exec_prepared"),
    pointers.data(), args.lengths.data(), args.binaries.data(), 0)};
  auto const r{make_result(pq_result, q)};
  get_notifs();
  return r;
}


void pqxx::connection::close()
{
  try
  {
    if (m_trans.get())
      process_notice(
        "Closing connection while " + m_trans.get()->description() +
        " is still open.");

    if (not m_receivers.empty())
    {
      process_notice("Closing connection with outstanding receivers.");
      m_receivers.clear();
    }

    std::list<errorhandler *> old_handlers;
    m_errorhandlers.swap(old_handlers);
    auto const rbegin{old_handlers.crbegin()}, rend{old_handlers.crend()};
    for (auto i{rbegin}; i != rend; ++i)
      pqxx::internal::gate::errorhandler_connection{**i}.unregister();

    PQfinish(m_conn);
    m_conn = nullptr;
  }
  catch (std::exception const &)
  {
    m_conn = nullptr;
    throw;
  }
}


int pqxx::connection::status() const noexcept
{
  return PQstatus(m_conn);
}


void pqxx::connection::register_transaction(transaction_base *t)
{
  m_trans.register_guest(t);
}


void pqxx::connection::unregister_transaction(transaction_base *t) noexcept
{
  try
  {
    m_trans.unregister_guest(t);
  }
  catch (std::exception const &e)
  {
    process_notice(e.what());
  }
}


std::pair<std::unique_ptr<char, std::function<void(char *)>>, std::size_t>
pqxx::connection::read_copy_line()
{
  using raw_line =
    std::pair<std::unique_ptr<char, std::function<void(char *)>>, std::size_t>;
  char *buf{nullptr};

  // Allocate once, re-use across invocations.
  static auto const q{std::make_shared<std::string>("[END COPY]")};

  auto const line_len{PQgetCopyData(m_conn, &buf, false)};
  switch (line_len)
  {
  case -2: // Error.
    throw failure{"Reading of table data failed: " + std::string{err_msg()}};

  case -1: // End of COPY.
    make_result(PQgetResult(m_conn), q);
    return raw_line{};

  case 0: // "Come back later."
    throw internal_error{"table read inexplicably went asynchronous"};

  default: // Success, got buffer size.
    // Line size includes a trailing zero, which we ignore.
    auto const text_len{static_cast<std::size_t>(line_len) - 1};
    return std::make_pair(
      std::unique_ptr<char, std::function<void(char *)>>{buf, PQfreemem},
      text_len);
  }
}


void pqxx::connection::write_copy_line(std::string_view line)
{
  static std::string const err_prefix{"Error writing to table: "};
  auto const size{check_cast<int>(line.size(), "write_copy_line()")};
  if (PQputCopyData(m_conn, line.data(), size) <= 0)
    throw failure{err_prefix + err_msg()};
  if (PQputCopyData(m_conn, "\n", 1) <= 0)
    throw failure{err_prefix + err_msg()};
}


void pqxx::connection::end_copy_write()
{
  int res{PQputCopyEnd(m_conn, nullptr)};
  switch (res)
  {
  case -1: throw failure{"Write to table failed: " + std::string{err_msg()}};
  case 0: throw internal_error{"table write is inexplicably asynchronous"};
  case 1:
    // Normal termination.  Retrieve result object.
    break;

  default:
    throw internal_error{
      "unexpected result " + to_string(res) + " from PQputCopyEnd()"};
  }

  static auto const q{std::make_shared<std::string>("[END COPY]")};
  make_result(PQgetResult(m_conn), q);
}


void pqxx::connection::start_exec(char const query[])
{
  if (PQsendQuery(m_conn, query) == 0)
    throw failure{err_msg()};
}


pqxx::internal::pq::PGresult *pqxx::connection::get_result()
{
  return PQgetResult(m_conn);
}


size_t pqxx::connection::esc_to_buf(std::string_view text, char *buf) const
{
  int err{0};
  auto const copied{
    PQescapeStringConn(m_conn, buf, text.data(), text.size(), &err)};
  if (err)
    throw argument_error{err_msg()};
  return copied;
}


std::string pqxx::connection::esc(std::string_view text) const
{
  std::string buf;
  buf.resize(2 * text.size() + 1);
  auto const copied{esc_to_buf(text, buf.data())};
  buf.resize(copied);
  return buf;
}


std::string
pqxx::connection::esc_raw(unsigned char const bin[], std::size_t len) const
{
  return pqxx::internal::esc_bin(
    std::string_view{reinterpret_cast<char const *>(bin), len});
}


std::string pqxx::connection::unesc_raw(char const text[]) const
{
  if (text[0] == '\\' and text[1] == 'x')
  {
    // Hex-escaped format.
    return pqxx::internal::unesc_bin(std::string_view{text});
  }
  else
  {
    // Legacy escape format.
    // TODO: Remove legacy support.
    std::size_t len;
    auto bytes{const_cast<unsigned char *>(
      reinterpret_cast<unsigned char const *>(text))};
    std::unique_ptr<unsigned char, std::function<void(unsigned char *)>> const
      ptr{PQunescapeBytea(bytes, &len), PQfreemem};
    return std::string{ptr.get(), ptr.get() + len};
  }
}


std::string
pqxx::connection::quote_raw(unsigned char const bin[], std::size_t len) const
{
  return "'" + esc_raw(bin, len) + "'::bytea";
}


std::string pqxx::connection::quote(binarystring const &b) const
{
  return quote_raw(b.data(), b.size());
}


std::string pqxx::connection::quote_name(std::string_view identifier) const
{
  std::unique_ptr<char, std::function<void(char *)>> buf{
    PQescapeIdentifier(m_conn, identifier.data(), identifier.size()),
    PQfreemem};
  if (buf.get() == nullptr)
    throw failure{err_msg()};
  return std::string{buf.get()};
}


std::string
pqxx::connection::esc_like(std::string_view text, char escape_char) const
{
  std::string out;
  out.reserve(text.size());
  internal::for_glyphs(
    internal::enc_group(encoding_id()),
    [&out, escape_char](char const *gbegin, char const *gend) {
      if ((gend - gbegin == 1) and (*gbegin == '_' or *gbegin == '%'))
        out.push_back(escape_char);

      for (; gbegin != gend; ++gbegin) out.push_back(*gbegin);
    },
    text.data(), text.size());
  return out;
}


namespace
{
// Convert a timeval to milliseconds, or -1 if no timeval is given.
[[maybe_unused]] constexpr int tv_milliseconds(timeval *tv = nullptr)
{
  if (tv == nullptr)
    return -1;
  else
    return pqxx::check_cast<int>(
      tv->tv_sec * 1000 + tv->tv_usec / 1000, "milliseconds");
}


/// Wait for an fd to become free for reading/writing.  Optional timeout.
void wait_fd(int fd, bool forwrite = false, timeval *tv = nullptr)
{
  if (fd < 0)
    throw pqxx::broken_connection{"No connection."};

// WSAPoll is available in winsock2.h only for versions of Windows >= 0x0600
#if defined(_WIN32) && (_WIN32_WINNT >= 0x0600)
  short const events{forwrite ? POLLWRNORM : POLLRDNORM};
  WSAPOLLFD fdarray{SOCKET(fd), events, 0};
  WSAPoll(&fdarray, 1, tv_milliseconds(tv));
  // TODO: Check for errors.
#elif defined(PQXX_HAVE_POLL)
  auto const events{static_cast<short>(
    POLLERR | POLLHUP | POLLNVAL | (forwrite ? POLLOUT : POLLIN))};
  pollfd pfd{fd, events, 0};
  poll(&pfd, 1, tv_milliseconds(tv));
  // TODO: Check for errors.
#else
  // No poll()?  Our last option is select().
  fd_set read_fds;
  FD_ZERO(&read_fds);
  if (not forwrite)
    FD_SET(fd, &read_fds);

  fd_set write_fds;
  FD_ZERO(&write_fds);
  if (forwrite)
    FD_SET(fd, &write_fds);

  fd_set except_fds;
  FD_ZERO(&except_fds);
  FD_SET(fd, &except_fds);

  select(fd + 1, &read_fds, &write_fds, &except_fds, tv);
  // TODO: Check for errors.
#endif
}
} // namespace

void pqxx::internal::wait_read(internal::pq::PGconn const *c)
{
  wait_fd(socket_of(c));
}


void pqxx::internal::wait_read(
  internal::pq::PGconn const *c, std::time_t seconds, long microseconds)
{
  // Not all platforms have suseconds_t for the microseconds.  And some 64-bit
  // systems use 32-bit integers here.
  timeval tv{
    check_cast<decltype(timeval::tv_sec)>(seconds, "read timeout seconds"),
    check_cast<decltype(timeval::tv_usec)>(
      microseconds, "read timeout microseconds")};
  wait_fd(socket_of(c), false, &tv);
}


void pqxx::internal::wait_write(internal::pq::PGconn const *c)
{
  wait_fd(socket_of(c), true);
}


void pqxx::connection::wait_read() const
{
  internal::wait_read(m_conn);
}


void pqxx::connection::wait_read(std::time_t seconds, long microseconds) const
{
  internal::wait_read(m_conn, seconds, microseconds);
}


int pqxx::connection::await_notification()
{
  int notifs = get_notifs();
  if (notifs == 0)
  {
    wait_read();
    notifs = get_notifs();
  }
  return notifs;
}


int pqxx::connection::await_notification(
  std::time_t seconds, long microseconds)
{
  int notifs = get_notifs();
  if (notifs == 0)
  {
    wait_read(seconds, microseconds);
    return get_notifs();
  }
  return notifs;
}


std::string pqxx::connection::adorn_name(std::string_view n)
{
  auto const id{to_string(++m_unique_id)};
  if (n.empty())
  {
    return "x" + id;
  }
  else
  {
    std::string name;
    name.reserve(n.size() + 1 + id.size());
    name.append(n);
    name.push_back('_');
    name.append(id);
    return name;
  }
}


std::string pqxx::connection::get_client_encoding() const
{
  return internal::name_encoding(encoding_id());
}


void pqxx::connection::set_client_encoding(char const encoding[])
{
  switch (auto const retval{PQsetClientEncoding(m_conn, encoding)}; retval)
  {
  case 0:
    // OK.
    break;
  case -1:
    if (is_open())
      throw failure{"Setting client encoding failed."};
    else
      throw broken_connection{"Lost connection to the database server."};
  default:
    throw internal_error{
      "Unexpected result from PQsetClientEncoding: " + to_string(retval)};
  }
}


int pqxx::connection::encoding_id() const
{
  int const enc{PQclientEncoding(m_conn)};
  if (enc == -1)
  {
    // PQclientEncoding does not query the database, but it does check for
    // broken connections.  And unfortunately, we check the encoding right
    // *before* checking a query result for failure.  So, we need to handle
    // connection failure here and it will apply in lots of places.
    // TODO: Make pqxx::result::result(...) do all the checking.
    if (is_open())
      throw failure{"Could not obtain client encoding."};
    else
      throw broken_connection{"Lost connection to the database server."};
  }
  return enc;
}


pqxx::result pqxx::connection::exec_params(
  std::string_view query, internal::params const &args)
{
  auto const pointers{args.get_pointers()};
  auto const q{std::make_shared<std::string>(query)};
  auto const nonnulls{
    check_cast<int>(args.nonnulls.size(), "exec_params() parameters")};
  auto const pq_result{PQexecParams(
    m_conn, q->c_str(), nonnulls, nullptr, pointers.data(),
    args.lengths.data(), args.binaries.data(), 0)};
  auto const r{make_result(pq_result, q)};
  get_notifs();
  return r;
}


namespace
{
/// Get the prevailing default value for a connection parameter.
char const *get_default(PQconninfoOption const &opt) noexcept
{
  if (opt.envvar == nullptr)
  {
    // There's no environment variable for this setting.  The only default is
    // the one that was compiled in.
    return opt.compiled;
  }
  // As of C++11, std::getenv() uses thread-local storage, so it should be
  // thread-safe.  MSVC still warns about it though.
  char const *var{std::getenv(opt.envvar)};
  if (var == nullptr)
  {
    // There's an environment variable for this setting, but it's not set.
    return opt.compiled;
  }

  // The environment variable is the prevailing default.
  return var;
}
} // namespace


std::string pqxx::connection::connection_string() const
{
  if (m_conn == nullptr)
    throw usage_error{"Can't get connection string: connection is not open."};

  std::unique_ptr<
    PQconninfoOption, std::function<void(PQconninfoOption *)>> const params{
    PQconninfo(m_conn), PQconninfoFree};
  if (params.get() == nullptr)
    throw std::bad_alloc{};

  std::string buf;
  for (std::size_t i{0}; params.get()[i].keyword != nullptr; ++i)
  {
    auto const param{params.get()[i]};
    if (param.val != nullptr)
    {
      auto const default_val{get_default(param)};
      if (
        (default_val == nullptr) or (std::strcmp(param.val, default_val) != 0))
      {
        if (not buf.empty())
          buf.push_back(' ');
        buf += param.keyword;
        buf.push_back('=');
        buf += param.val;
      }
    }
  }
  return buf;
}
