/** Implementation of the pqxx::connection class.
 *
 * pqxx::connection encapsulates a connection to a database.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
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
#include "libpq-fe.h"
}

#include "pqxx/binarystring"
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
  void pqxx_notice_processor(void *conn, const char *msg) noexcept
  {
    reinterpret_cast<pqxx::connection *>(conn)->process_notice(msg);
  }


  // There's no way in libpq to disable a connection's notice processor.  So,
  // set an inert one to get the same effect.
  void inert_notice_processor(void *, const char *) noexcept {}
} // extern "C"


std::string pqxx::encrypt_password(const char user[], const char password[])
{
  std::unique_ptr<char, std::function<void(char *)>> p{
    PQencryptPassword(password, user),
    pqxx::internal::freepqmem_templated<char>};
  return std::string{p.get()};
}


pqxx::connection::connection(connection &&rhs) :
        m_conn{rhs.m_conn},
        m_unique_id{rhs.m_unique_id}
{
  rhs.check_movable();
  rhs.m_conn = nullptr;
}


void pqxx::connection::init(const char options[])
{
  m_conn = PQconnectdb(options);
  if (m_conn == nullptr)
    throw std::bad_alloc{};
  try
  {
    if (PQstatus(m_conn) != CONNECTION_OK)
      throw broken_connection{PQerrorMessage(m_conn)};

    set_up_state();
  }
  catch (const std::exception &)
  {
    PQfinish(m_conn);
    throw;
  }
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
  internal::pq::PGresult *rhs, const std::string &query)
{
  return pqxx::internal::gate::result_creation::create(
    rhs, query, internal::enc_group(encoding_id()));
}


int pqxx::connection::backendpid() const noexcept
{
  return m_conn ? PQbackendPID(m_conn) : 0;
}


namespace
{
PQXX_PURE int socket_of(const ::pqxx::internal::pq::PGconn *c) noexcept
{
  return c ? PQsocket(c) : -1;
}
} // namespace


int pqxx::connection::sock() const noexcept
{
  return socket_of(m_conn);
}


int pqxx::connection::protocol_version() const noexcept
{
  return m_conn ? PQprotocolVersion(m_conn) : 0;
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
  read_capabilities();

  // The default notice processor in libpq writes to stderr.  Ours does
  // nothing.
  // If the caller registers an error handler, this gets replaced with an
  // error handler that walks down the connection's chain of handlers.  We
  // don't do that by default because there's a danger: libpq may call the
  // notice processor via a result object, even after the connection has been
  // destroyed and the handlers list no longer exists.
  clear_notice_processor();
}


void pqxx::connection::check_result(const result &R)
{
  // A shame we can't quite detect out-of-memory to turn this into a bad_alloc!
  if (not pqxx::internal::gate::result_connection{R})
    throw failure(err_msg());
  pqxx::internal::gate::result_creation{R}.check_status();
}


bool pqxx::connection::is_open() const noexcept
{
  return status() == CONNECTION_OK;
}


void pqxx::connection::process_notice_raw(const char msg[]) noexcept
{
  if ((msg == nullptr) or (*msg == '\0'))
    return;
  const auto rbegin = m_errorhandlers.crbegin(),
             rend = m_errorhandlers.crend();
  for (auto i = rbegin; (i != rend) and (**i)(msg); ++i)
    ;
}


void pqxx::connection::process_notice(const char msg[]) noexcept
{
  if (msg == nullptr)
    return;
  const auto len = strlen(msg);
  if (len == 0)
    return;
  if (msg[len - 1] == '\n')
  {
    process_notice_raw(msg);
  }
  else
    try
    {
      // Newline is missing.  Try the C++ string version of this function.
      process_notice(std::string{msg});
    }
    catch (const std::exception &)
    {
      // If we can't even do that, use plain old buffer copying instead
      // (unavoidably, this will break up overly long messages!)
      const char separator[] = "[...]\n";
      char buf[1007];
      size_t bytes = sizeof(buf) - sizeof(separator) - 1;
      size_t written;
      memcpy(&buf[bytes], separator, sizeof(separator));
      // Write all chunks but last.  Each will fill the buffer exactly.
      for (written = 0; (written + bytes) < len; written += bytes)
      {
        memcpy(buf, &msg[written], bytes);
        process_notice_raw(buf);
      }
      // Write any remaining bytes (which won't fill an entire buffer)
      bytes = len - written;
      memcpy(buf, &msg[written], bytes);
      // Add trailing nul byte, plus newline unless there already is one
      if (buf[bytes - 1] != '\n')
      {
        buf[bytes++] = '\n';
        buf[bytes++] = '\0';
      }
      process_notice_raw(buf);
    }
}


void pqxx::connection::process_notice(const std::string &msg) noexcept
{
  // Ensure that message passed to errorhandler ends in newline
  if (msg[msg.size() - 1] == '\n')
    process_notice_raw(msg.c_str());
  else
    try
    {
      const std::string nl = msg + "\n";
      process_notice_raw(nl.c_str());
    }
    catch (const std::exception &)
    {
      // If nothing else works, try writing the message without the newline
      process_notice_raw(msg.c_str());
      // This is ugly.
      process_notice_raw("\n");
    }
}


void pqxx::connection::trace(FILE *Out) noexcept
{
  if (m_conn)
  {
    if (Out)
      PQtrace(m_conn, Out);
    else
      PQuntrace(m_conn);
  }
}


void pqxx::connection::add_receiver(pqxx::notification_receiver *T)
{
  if (T == nullptr)
    throw argument_error{"Null receiver registered"};

  // Add to receiver list and attempt to start listening.
  const auto p = m_receivers.find(T->channel());
  const auto new_value = receiver_list::value_type{T->channel(), T};

  if (p == m_receivers.end())
  {
    // Not listening on this event yet, start doing so.
    const std::string LQ{"LISTEN " + quote_name(T->channel())};
    check_result(make_result(PQexec(m_conn, LQ.c_str()), LQ));
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
    auto needle =
      std::pair<const std::string, notification_receiver *>{T->channel(), T};
    auto R = m_receivers.equal_range(needle.first);
    auto i = find(R.first, R.second, needle);

    if (i == R.second)
    {
      process_notice(
        "Attempt to remove unknown receiver '" + needle.first + "'");
    }
    else
    {
      // Erase first; otherwise a notification for the same receiver may yet
      // come in and wreak havoc.  Thanks Dragan Milenkovic.
      const bool gone = (R.second == ++R.first);
      m_receivers.erase(i);
      if (gone)
        exec(("UNLISTEN " + quote_name(needle.first)).c_str());
    }
  }
  catch (const std::exception &e)
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


namespace
{
/// Stateful libpq "cancel" operation.
class cancel_wrapper
{
  PGcancel *m_cancel;
  char m_errbuf[500];

public:
  explicit cancel_wrapper(PGconn *conn) : m_cancel{nullptr}, m_errbuf{}
  {
    if (conn)
    {
      m_cancel = PQgetCancel(conn);
      if (m_cancel == nullptr)
        throw std::bad_alloc{};
    }
  }
  ~cancel_wrapper()
  {
    if (m_cancel)
      PQfreeCancel(m_cancel);
  }

  void operator()()
  {
    if (not m_cancel)
      return;
    if (PQcancel(m_cancel, m_errbuf, int{sizeof(m_errbuf)}) == 0)
      throw pqxx::sql_error{std::string{m_errbuf}};
  }
};
} // namespace


void pqxx::connection::cancel_query()
{
  cancel_wrapper cancel{m_conn};
  cancel();
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
  return notify_ptr(
    PQnotifies(conn), pqxx::internal::freepqmem_templated<PGnotify>);
}
} // namespace


int pqxx::connection::get_notifs()
{
  if (not consume_input())
    throw broken_connection{"Connection lost."};

  // Even if somehow we receive notifications during our transaction, don't
  // deliver them.
  if (m_trans.get())
    return 0;

  int notifs = 0;
  for (auto N = get_notif(m_conn); N.get(); N = get_notif(m_conn))
  {
    notifs++;

    const auto Hit = m_receivers.equal_range(std::string{N->relname});
    for (auto i = Hit.first; i != Hit.second; ++i) try
      {
        (*i->second)(N->extra, N->be_pid);
      }
      catch (const std::exception &e)
      {
        try
        {
          process_notice(
            "Exception in notification receiver '" + i->first +
            "': " + e.what() + "\n");
        }
        catch (const std::bad_alloc &)
        {
          // Out of memory.  Try to get the message out in a more robust way.
          process_notice(
            "Exception in notification receiver, "
            "and also ran out of memory\n");
        }
        catch (const std::exception &)
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


const char *pqxx::connection::dbname() const
{
  return PQdb(m_conn);
}


const char *pqxx::connection::username() const
{
  return PQuser(m_conn);
}


const char *pqxx::connection::hostname() const
{
  return PQhost(m_conn);
}


const char *pqxx::connection::port() const
{
  return PQport(m_conn);
}


const char *pqxx::connection::err_msg() const noexcept
{
  return m_conn ? PQerrorMessage(m_conn) : "No connection to database";
}


void pqxx::connection::clear_notice_processor()
{
  PQsetNoticeProcessor(m_conn, inert_notice_processor, nullptr);
}


void pqxx::connection::set_notice_processor()
{
  PQsetNoticeProcessor(m_conn, pqxx_notice_processor, this);
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
    set_notice_processor();
  m_errorhandlers.push_back(handler);
}


void pqxx::connection::unregister_errorhandler(errorhandler *handler) noexcept
{
  // The errorhandler itself will take care of nulling its pointer to this
  // connection.
  m_errorhandlers.remove(handler);
  if (m_errorhandlers.empty())
    clear_notice_processor();
}


std::vector<pqxx::errorhandler *> pqxx::connection::get_errorhandlers() const
{
  return std::vector<errorhandler *>{std::begin(m_errorhandlers),
                                     std::end(m_errorhandlers)};
}


pqxx::result pqxx::connection::exec(const char Query[])
{
  auto R = make_result(PQexec(m_conn, Query), Query);
  check_result(R);

  get_notifs();
  return R;
}


#if defined(PQXX_HAVE_PQENCRYPTPASSWORDCONN)
std::string pqxx::connection::encrypt_password(
  const char user[], const char password[], const char *algorithm)
{
  const auto buf = PQencryptPasswordConn(m_conn, user, password, algorithm);
  std::unique_ptr<const char, std::function<void(const char *)>> ptr{
    buf, pqxx::internal::freepqmem_templated<const char>};
  return std::string(ptr.get());
}
#endif // PQXX_HAVE_PQENCRYPTPASSWORDCONN


void pqxx::connection::prepare(const char name[], const char definition[])
{
  auto r =
    make_result(PQprepare(m_conn, name, definition, 0, nullptr), "[PREPARE]");
  check_result(r);
}


void pqxx::connection::prepare(const char definition[])
{
  this->prepare("", definition);
}


void pqxx::connection::unprepare(std::string_view name)
{
  exec(("DEALLOCATE " + quote_name(name)).c_str());
}


pqxx::result pqxx::connection::exec_prepared(
  const std::string &statement, const internal::params &args)
{
  const auto pointers = args.get_pointers();
  const auto pq_result = PQexecPrepared(
    m_conn, statement.c_str(),
    check_cast<int>(args.nonnulls.size(), "exec_prepared"), pointers.data(),
    args.lengths.data(), args.binaries.data(), 0);
  const auto r = make_result(pq_result, statement);
  check_result(r);
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
    const auto rbegin = old_handlers.crbegin(), rend = old_handlers.crend();
    for (auto i = rbegin; i != rend; ++i)
      pqxx::internal::gate::errorhandler_connection{**i}.unregister();

    PQfinish(m_conn);
    m_conn = nullptr;
  }
  catch (const std::exception &)
  {
    m_conn = nullptr;
    throw;
  }
}


int pqxx::connection::status() const noexcept
{
  return PQstatus(m_conn);
}


void pqxx::connection::register_transaction(transaction_base *T)
{
  m_trans.register_guest(T);
}


void pqxx::connection::unregister_transaction(transaction_base *T) noexcept
{
  try
  {
    m_trans.unregister_guest(T);
  }
  catch (const std::exception &e)
  {
    process_notice(e.what());
  }
}


bool pqxx::connection::read_copy_line(std::string &Line)
{
  Line.erase();
  bool Result;

  char *Buf = nullptr;
  const std::string query = "[END COPY]";
  const auto line_len = PQgetCopyData(m_conn, &Buf, false);
  switch (line_len)
  {
  case -2:
    throw failure{"Reading of table data failed: " + std::string{err_msg()}};

  case -1:
    for (auto R = make_result(PQgetResult(m_conn), query);
         pqxx::internal::gate::result_connection(R);
         R = make_result(PQgetResult(m_conn), query))
      check_result(R);
    Result = false;
    break;

  case 0: throw internal_error{"table read inexplicably went asynchronous"};

  default:
    if (Buf)
    {
      std::unique_ptr<char, std::function<void(char *)>> PQA(
        Buf, pqxx::internal::freepqmem_templated<char>);
      Line.assign(Buf, unsigned(line_len));
    }
    Result = true;
  }

  return Result;
}


void pqxx::connection::write_copy_line(std::string_view line)
{
  static const std::string err_prefix{"Error writing to table: "};
  const auto size{check_cast<int>(line.size(), "write_copy_line()")};
  if (PQputCopyData(m_conn, line.data(), size) <= 0)
    throw failure{err_prefix + err_msg()};
  if (PQputCopyData(m_conn, "\n", 1) <= 0)
    throw failure{err_prefix + err_msg()};
}


void pqxx::connection::end_copy_write()
{
  int Res = PQputCopyEnd(m_conn, nullptr);
  switch (Res)
  {
  case -1: throw failure{"Write to table failed: " + std::string{err_msg()}};
  case 0: throw internal_error{"table write is inexplicably asynchronous"};
  case 1:
    // Normal termination.  Retrieve result object.
    break;

  default:
    throw internal_error{"unexpected result " + to_string(Res) +
                         " from PQputCopyEnd()"};
  }

  check_result(make_result(PQgetResult(m_conn), "[END COPY]"));
}


void pqxx::connection::start_exec(const char query[])
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
  int err = 0;
  const auto copied =
    PQescapeStringConn(m_conn, buf, text.data(), text.size(), &err);
  if (err)
    throw argument_error{err_msg()};
  return copied;
}


std::string pqxx::connection::esc(std::string_view text) const
{
  std::string buf;
  buf.resize(2 * text.size() + 1);
  const auto copied = esc_to_buf(text, buf.data());
  buf.resize(copied);
  return buf;
}


std::string
pqxx::connection::esc_raw(const unsigned char bin[], size_t len) const
{
  size_t bytes = 0;

  std::unique_ptr<unsigned char, std::function<void(unsigned char *)>> buf{
    PQescapeByteaConn(m_conn, bin, len, &bytes),
    pqxx::internal::freepqmem_templated<unsigned char>};
  if (buf.get() == nullptr)
    throw std::bad_alloc{};
  return std::string{reinterpret_cast<char *>(buf.get())};
}


std::string pqxx::connection::unesc_raw(const char text[]) const
{
  size_t len;
  unsigned char *bytes =
    const_cast<unsigned char *>(reinterpret_cast<const unsigned char *>(text));
  const std::unique_ptr<unsigned char, std::function<void(unsigned char *)>>
    ptr{PQunescapeBytea(bytes, &len),
        internal::freepqmem_templated<unsigned char>};
  return std::string{ptr.get(), ptr.get() + len};
}


std::string
pqxx::connection::quote_raw(const unsigned char bin[], size_t len) const
{
  return "'" + esc_raw(bin, len) + "'::bytea";
}


std::string pqxx::connection::quote(const binarystring &b) const
{
  return quote_raw(b.data(), b.size());
}


std::string pqxx::connection::quote_name(std::string_view identifier) const
{
  std::unique_ptr<char, std::function<void(char *)>> buf{
    PQescapeIdentifier(m_conn, identifier.data(), identifier.size()),
    pqxx::internal::freepqmem_templated<char>};
  if (buf.get() == nullptr)
    throw failure{err_msg()};
  return std::string{buf.get()};
}


std::string
pqxx::connection::esc_like(std::string_view bin, char escape_char) const
{
  std::string out;
  out.reserve(bin.size());
  internal::for_glyphs(
    internal::enc_group(encoding_id()),
    [&out, escape_char](const char *gbegin, const char *gend) {
      if ((gend - gbegin == 1) and (*gbegin == '_' or *gbegin == '%'))
        out.push_back(escape_char);

      for (; gbegin != gend; ++gbegin) out.push_back(*gbegin);
    },
    bin.data(), bin.size());
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
  const short events = (forwrite ? POLLWRNORM : POLLRDNORM);
  WSAPOLLFD fdarray{SOCKET(fd), events, 0};
  WSAPoll(&fdarray, 1, tv_milliseconds(tv));
  // TODO: Check for errors.
#elif defined(PQXX_HAVE_POLL)
  const short events =
    short(POLLERR | POLLHUP | POLLNVAL | (forwrite ? POLLOUT : POLLIN));
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

void pqxx::internal::wait_read(const internal::pq::PGconn *c)
{
  wait_fd(socket_of(c));
}


void pqxx::internal::wait_read(
  const internal::pq::PGconn *c, long seconds, long microseconds)
{
  // These are really supposed to be time_t and suseconds_t.  But not all
  // platforms have that type; some use "long" instead, and some 64-bit
  // systems use 32-bit integers here.
  timeval tv{
    check_cast<decltype(timeval::tv_sec)>(seconds, "read timeout seconds"),
    check_cast<decltype(timeval::tv_usec)>(
      microseconds, "read timeout microseconds")};
  wait_fd(socket_of(c), false, &tv);
}


void pqxx::internal::wait_write(const internal::pq::PGconn *c)
{
  wait_fd(socket_of(c), true);
}


void pqxx::connection::wait_read() const
{
  internal::wait_read(m_conn);
}


void pqxx::connection::wait_read(long seconds, long microseconds) const
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


int pqxx::connection::await_notification(long seconds, long microseconds)
{
  int notifs = get_notifs();
  if (notifs == 0)
  {
    wait_read(seconds, microseconds);
    notifs = get_notifs();
  }
  return notifs;
}


void pqxx::connection::read_capabilities()
{
  if (const auto proto_ver = protocol_version(); proto_ver < 3)
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
}


std::string pqxx::connection::adorn_name(std::string_view n)
{
  const std::string id = to_string(++m_unique_id);
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


void pqxx::connection::set_client_encoding(const char encoding[])
{
  switch (const auto retval = PQsetClientEncoding(m_conn, encoding); retval)
  {
  case 0:
    // OK.
    break;
  case -1:
    // TODO: Any helpful information we could give here?
    throw failure{"Setting client encoding failed."};
  default:
    throw internal_error{"Unexpected result from PQsetClientEncoding: " +
                         to_string(retval)};
  }
}


int pqxx::connection::encoding_id() const
{
  const int enc = PQclientEncoding(m_conn);
  if (enc == -1)
    throw failure{"Could not obtain client encoding."};
  return enc;
}


pqxx::result pqxx::connection::exec_params(
  const std::string &query, const internal::params &args)
{
  const auto pointers = args.get_pointers();
  const auto pq_result = PQexecParams(
    m_conn, query.c_str(),
    check_cast<int>(args.nonnulls.size(), "exec_params() parameters"), nullptr,
    pointers.data(), args.lengths.data(), args.binaries.data(), 0);
  const auto r = make_result(pq_result, query);
  check_result(r);
  get_notifs();
  return r;
}
