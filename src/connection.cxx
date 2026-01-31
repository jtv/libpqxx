/** Implementation of the pqxx::connection class.
 *
 * pqxx::connection encapsulates a connection to a database.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

// For ioctlsocket().
// We would normally include this after the standard headers, but MinGW warns
// that we can't include windows.h before winsock2.h, and some of the standard
// headers probably include windows.h.
#if defined(_WIN32) && __has_include(<winsock2.h>)
#  include <winsock2.h>
#endif

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>

// For fcntl().
#if __has_include(<fcntl.h>)
#  include <fcntl.h>
#endif
#if __has_include(<unistd.h>)
#  include <unistd.h>
#endif

#include "pqxx/internal/header-pre.hxx"

extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/internal/wait.hxx"
#include "pqxx/nontransaction.hxx"
#include "pqxx/notification.hxx"
#include "pqxx/pipeline.hxx"
#include "pqxx/result.hxx"
#include "pqxx/strconv.hxx"
#include "pqxx/transaction.hxx"

#include "pqxx/internal/gates/errorhandler-connection.hxx"
#include "pqxx/internal/gates/result-connection.hxx"
#include "pqxx/internal/gates/result-creation.hxx"

#include "pqxx/internal/header-post.hxx"


namespace
{
void process_notice_raw(
  pqxx::internal::notice_waiters *waiters, pqxx::zview msg) noexcept
{
  if ((waiters != nullptr) and not msg.empty())
  {
    auto const rbegin = std::crbegin(waiters->errorhandlers),
               rend = std::crend(waiters->errorhandlers);
    for (auto i{rbegin}; (i != rend) and (**i)(msg.data()); ++i);

    if (waiters->notice_handler)
      waiters->notice_handler(msg);
  }
}
} // namespace


extern "C"
{
  // The PQnoticeProcessor that receives an error or warning from libpq and
  // sends it to the appropriate connection for processing.
  PQXX_ZARGS void pqxx_notice_processor(void *cx, char const *msg) noexcept
  {
    process_notice_raw(
      reinterpret_cast<pqxx::internal::notice_waiters *>(cx),
      pqxx::zview{msg});
  }
} // extern "C"


using namespace std::literals;


PQXX_COLD PQXX_LIBEXPORT void pqxx::internal::skip_init_ssl(int skips) noexcept
{
  // We got "skip flags," but we pass to libpq which libraries we *do* want it
  // to initialise.
  PQinitOpenSSL(
    (skips & (1 << skip_init::openssl)) == 0,
    (skips & (1 << skip_init::crypto)) == 0);
}


pqxx::connection::connection(connection &&rhs, sl loc) :
        m_conn{rhs.m_conn},
        m_notice_waiters{std::move(rhs.m_notice_waiters)},
        m_notification_handlers{std::move(rhs.m_notification_handlers)},
        m_created_loc{loc},
        m_unique_id{rhs.m_unique_id}
{
  rhs.check_movable(loc);
  rhs.m_conn = nullptr;
}


pqxx::connection::connection(
  connection::connect_mode, zview connection_string, sl loc) :
        m_conn{PQconnectStart(connection_string.c_str())}, m_created_loc{loc}
{
  if (m_conn == nullptr)
    throw std::bad_alloc{};

  set_up_notice_handlers();

  if (status() == CONNECTION_BAD)
  {
    std::string const msg{PQerrorMessage(m_conn)};
    PQfinish(m_conn);
    m_conn = nullptr;
    throw pqxx::broken_connection{msg, loc};
  }
}


pqxx::connection::connection(internal::pq::PGconn *raw_conn, sl loc) :
        m_conn{raw_conn}, m_created_loc{loc}
{
  set_up_notice_handlers();
}


std::pair<bool, bool> pqxx::connection::poll_connect(sl loc)
{
  switch (PQconnectPoll(m_conn))
  {
  case PGRES_POLLING_FAILED:
    throw pqxx::broken_connection{PQerrorMessage(m_conn), loc};
  case PGRES_POLLING_READING: return std::make_pair(true, false);
  case PGRES_POLLING_WRITING: return std::make_pair(false, true);
  case PGRES_POLLING_OK:
    if (not is_open())
      throw pqxx::broken_connection{PQerrorMessage(m_conn), loc};
    [[likely]] return std::make_pair(false, false);
  case PGRES_POLLING_ACTIVE:
    throw internal_error{
      "Nonblocking connection poll returned obsolete 'active' state.", loc};
  default:
    throw internal_error{
      "Nonblocking connection poll returned unknown value.", loc};
  }
}


void pqxx::connection::set_up_notice_handlers()
{
  if (not m_notice_waiters)
    m_notice_waiters = std::make_shared<pqxx::internal::notice_waiters>();

  // Our notice processor gets a pointer to our notice_waiters.  We can't
  // just pass "this" to it, because it may get called at a time when the
  // pqxx::connection has already been destroyed and only a pqxx::result
  // remains.
  if (m_conn != nullptr)
    PQsetNoticeProcessor(
      m_conn, pqxx_notice_processor, m_notice_waiters.get());
}


void pqxx::connection::complete_connection(sl loc)
{
  if (m_conn == nullptr)
    throw std::bad_alloc{};
  try
  {
    if (not is_open())
      throw broken_connection{PQerrorMessage(m_conn), loc};

    constexpr int min_proto{3};
    if (auto const proto_ver{protocol_version()}; proto_ver < min_proto)
    {
      if (proto_ver == 0)
        throw broken_connection{"No connection.", loc};
      else
        throw version_mismatch{
          std::format(
            "Unsupported frontend/backend protocol version; {} is the "
            "minimum.",
            min_proto),
          loc};
    }

    constexpr int min_server{110'000};
    if (server_version() < min_server)
      throw version_mismatch{
        std::format(
          "Unsupported server version; the minimum is {}.{}.",
          min_server / 10'000, min_server % 10'000),
        loc};
  }
  catch (std::exception const &)
  {
    PQfinish(m_conn);
    m_conn = nullptr;
    throw;
  }
}


namespace
{
/// Return value-terminated array of `PQconninfoOption` into a `std::range`.
/** The termination condition is the `keyword` field being null.
 */
std::span<PQconninfoOption const>
span_options(PQconninfoOption const *opts) noexcept
{
  PQconninfoOption const *here{opts};
  while (here->keyword != nullptr) ++here;
  return {opts, static_cast<std::size_t>(here - opts)};
}


/// If `opts` is not null, free the options array to which it points.
void delete_pg_conn_option(pqxx::internal::pg_conn_option opts[]) noexcept
{
  PQconninfoFree(reinterpret_cast<PQconninfoOption *>(opts));
}
} // namespace


namespace pqxx::internal
{
connection_string_parser::connection_string_parser(
  char const connection_string[], sl loc) :
        m_options{nullptr, delete_pg_conn_option}
{
  if ((connection_string != nullptr) and (connection_string[0] != '\0'))
  {
    char *errmsg{nullptr};
    m_options = decltype(m_options){
      reinterpret_cast<pqxx::internal::pg_conn_option *>(
        PQconninfoParse(connection_string, &errmsg)),
      delete_pg_conn_option};
    std::unique_ptr<char[], decltype(&pqxx::internal::pq::pqfreemem)> const
      errmsg_guard{errmsg, pqxx::internal::pq::pqfreemem};
    if (not m_options)
    {
      if (errmsg)
        throw broken_connection{
          std::format("Error in connection string: {}", errmsg), loc};
      else
        throw broken_connection{"Could not parse connection string.", loc};
    }
  }
}


connection_string_parser::~connection_string_parser() noexcept = default;


std::array<std::vector<char const *>, 2u>
connection_string_parser::parse() const
{
  std::array<std::vector<char const *>, 2u> output{};
  if (m_options)
  {
    // Was a value specified for this connection option?
    static constexpr auto has_value{
      [](PQconninfoOption const &o) { return o.val != nullptr; }};

    // A span across the parsed connection string options.
    auto const parsed{span_options(
      reinterpret_cast<PQconninfoOption const *>(m_options.get()))};

    // This will allocate room for all possible options, since the array of
    // PQconninfoOption we got from libpq includes options that were not
    // specified (though their "val" will be null and we won't include them in
    // our output).  It also includes room for a terminating null.
    //
    // The generous allocation is probably no big deal though since there are
    // only so many different connection options.  It also saves us having to
    // worry about reserving space when we add in any key/value parameters
    // later.
    std::get<0>(output).reserve(std::size(parsed));
    std::get<1>(output).reserve(std::size(parsed));

    for (auto const &o : parsed | std::ranges::views::filter(has_value))
    {
      std::get<0>(output).push_back(o.keyword);
      std::get<1>(output).push_back(o.val);
    }
  }
  return output;
}
} // namespace pqxx::internal


void pqxx::connection::init(
  std::vector<const char *> const &keys,
  std::vector<const char *> const &values, sl loc)
{
  m_conn = PQconnectdbParams(keys.data(), values.data(), 0);
  set_up_notice_handlers();
  complete_connection(loc);
}


void pqxx::connection::check_movable(sl loc) const
{
  if (m_trans)
    throw pqxx::usage_error{
      "Moving a connection with a transaction open.", loc};
  if (not std::empty(m_receivers))
    throw pqxx::usage_error{
      "Moving a connection with notification receivers registered.", loc};
}


void pqxx::connection::check_overwritable(sl loc) const
{
  if (m_trans)
    throw pqxx::usage_error{
      "Moving a connection onto one with a transaction open.", loc};
  if (not std::empty(m_receivers))
    throw usage_error{
      "Moving a connection onto one "
      "with notification receivers registered.",
      loc};
}


pqxx::connection &pqxx::connection::operator=(connection &&rhs)
{
  check_overwritable(m_created_loc);
  rhs.check_movable(m_created_loc);

  // Close our old connection, if any.
  close(m_created_loc);

  m_conn = std::exchange(rhs.m_conn, nullptr);
  m_unique_id = rhs.m_unique_id;
  m_notice_waiters = std::move(rhs.m_notice_waiters);
  m_notification_handlers = std::move(rhs.m_notification_handlers);
  m_created_loc = rhs.m_created_loc;

  return *this;
}


pqxx::result pqxx::connection::make_result(
  internal::pq::PGresult *pgr, std::shared_ptr<std::string> const &query,
  std::string_view desc, sl loc)
{
  std::shared_ptr<internal::pq::PGresult> const smart{
    pgr, internal::clear_result};
  if (not smart)
  {
    if (is_open())
      throw failure(err_msg());
    else
      throw broken_connection{"Lost connection to the database server.", loc};
  }
  auto const enc{get_encoding_group(loc)};
  auto r{pqxx::internal::gate::result_creation::create(
    smart, query, m_notice_waiters, enc)};
  pqxx::internal::gate::result_creation{r}.check_status(desc, loc);
  return r;
}


PQXX_COLD int pqxx::connection::backendpid() const & noexcept
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


int pqxx::connection::sock() const & noexcept
{
  return socket_of(m_conn);
}


PQXX_COLD int pqxx::connection::protocol_version() const noexcept
{
  return (m_conn == nullptr) ? 0 : PQprotocolVersion(m_conn);
}


PQXX_COLD int pqxx::connection::server_version() const noexcept
{
  return PQserverVersion(m_conn);
}


void pqxx::connection::set_variable(
  std::string_view var, std::string_view value, sl loc) &
{
  exec(std::format("SET {}={}", quote_name(var), value), loc);
}


std::string pqxx::connection::get_variable(std::string_view var, sl loc)
{
  return exec(std::format("SHOW {}", quote_name(var)), loc)
    .at(0)
    .at(0)
    .as(std::string{});
}


std::string pqxx::connection::get_var(std::string_view var, sl loc)
{
  // (Variables can't be null, so far as I can make out.)
  return exec(std::format("SHOW {}", quote_name(var)), loc)
    .one_field_ref(loc)
    .as<std::string>(loc);
}


bool pqxx::connection::is_open() const noexcept
{
  return status() == CONNECTION_OK;
}


void pqxx::connection::process_notice(char const msg[]) noexcept
{
  process_notice(zview{msg});
}


void pqxx::connection::process_notice(zview msg) noexcept
{
  if (not msg.empty())
    process_notice_raw(m_notice_waiters.get(), msg);
}


PQXX_COLD void pqxx::connection::trace(FILE *out) noexcept
{
  if (m_conn)
  {
    if (out)
      PQtrace(m_conn, out);
    else
      PQuntrace(m_conn);
  }
}


PQXX_COLD void
pqxx::connection::add_receiver(pqxx::notification_receiver *n, sl loc)
{
  if (n == nullptr)
    throw argument_error{"Null receiver registered", loc};

  // Add to receiver list and attempt to start listening.
  auto const p{m_receivers.find(n->channel())};
  auto const new_value{receiver_list::value_type{n->channel(), n}};

  if (p == std::end(m_receivers))
  {
    // Not listening on this event yet, start doing so.
    auto const lq{std::make_shared<std::string>(
      std::format("LISTEN {}", quote_name(n->channel())))};
    make_result(PQexec(m_conn, lq->c_str()), lq, *lq, loc);
    m_receivers.insert(new_value);
  }
  else
  {
    m_receivers.insert(p, new_value);
  }
}


void pqxx::connection::listen(
  std::string_view channel, notification_handler handler, sl loc)
{
  if (m_trans != nullptr)
    throw usage_error{
      std::format(
        "Attempting to listen for notifications on '{}' while transaction is "
        "active.",
        channel),
      loc};

  std::string str_name{channel};

  auto const pos{m_notification_handlers.lower_bound(str_name)},
    handlers_end{std::end(m_notification_handlers)};

  if (handler)
  {
    // Setting a handler.
    if ((pos != handlers_end) and (pos->first == channel))
    {
      // Overwrite existing handler.
      m_notification_handlers.insert_or_assign(
        pos, std::move(str_name), std::move(handler));
    }
    else
    {
      // We had no handler installed for this name.  Start listening.
      exec(std::format("LISTEN {}", quote_name(channel)), loc).no_rows(loc);
      m_notification_handlers.emplace_hint(pos, channel, std::move(handler));
    }
  }
  else
  {
    // Installing an empty handler.  That's equivalent to removing whatever
    // handler may have been installed previously.
    if (pos != handlers_end)
    {
      // Yes, we had a handler for this name.  Remove it.
      exec(std::format("UNLISTEN {}", quote_name(channel)), loc).no_rows(loc);
      m_notification_handlers.erase(pos);
    }
  }
}


PQXX_COLD void pqxx::connection::remove_receiver(
  pqxx::notification_receiver *T, sl loc) noexcept
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
      [[unlikely]] process_notice(std::format(
        "Attempt to remove unknown receiver '{}'\n", needle.first));
    }
    else
    {
      // Erase first; otherwise a notification for the same receiver may yet
      // come in and wreak havoc.  Thanks Dragan Milenkovic.
      bool const gone{R.second == ++R.first};
      m_receivers.erase(i);
      if (gone)
        exec(std::format("UNLISTEN {}", quote_name(needle.first)), loc);
    }
  }
  catch (std::exception const &e)
  {
    // TODO: Make at least an attempt to append a newline.
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
/// Wrapper for `PQfreeCancel`, with C++ linkage.
void wrap_pgfreecancel(PGcancel *ptr)
{
  PQfreeCancel(ptr);
}


/// A fairly arbitrary buffer size for error strings and such.
constexpr int buf_size{500u};
} // namespace


PQXX_COLD void pqxx::connection::cancel_query(sl loc)
{
  std::unique_ptr<PGcancel, void (*)(PGcancel *)> const cancel{
    PQgetCancel(m_conn), wrap_pgfreecancel};
  if (cancel == nullptr) [[unlikely]]
    throw std::bad_alloc{};

  std::array<char, buf_size> errbuf{};
  auto const err{errbuf.data()};
  auto const c{PQcancel(cancel.get(), err, buf_size)};
  if (c == 0) [[unlikely]]
    throw pqxx::sql_error{
      std::string{err, std::size(errbuf)}, "[cancel]", "", loc};
}


#if defined(_WIN32) || __has_include(<fcntl.h>)
void pqxx::connection::set_blocking(bool block, sl loc) &
{
  auto const fd{sock()};
#  if defined _WIN32
  unsigned long mode{not block};
  if (std::cmp_not_equal(
        ::ioctlsocket(static_cast<SOCKET>(fd), FIONBIO, &mode), 0))
  {
    std::array<char, buf_size> errbuf{};
    char const *err{pqxx::internal::error_string(WSAGetLastError(), errbuf)};
    throw broken_connection{
      std::format("Could not set socket's blocking mode: {}", err), loc};
  }
#  else  // _WIN32
  std::array<char, buf_size> errbuf{};
  auto flags{::fcntl(fd, F_GETFL, 0)};
  if (flags == -1)
  {
    char const *const err{pqxx::internal::error_string(errno, errbuf)};
    throw broken_connection{
      std::format("Could not get socket state: {}", err), loc};
  }
  if (block)
    flags &= ~O_NONBLOCK;
  else
    flags |= O_NONBLOCK;
  if (::fcntl(fd, F_SETFL, flags) == -1)
  {
    char const *const err{pqxx::internal::error_string(errno, errbuf)};
    throw broken_connection{
      std::format("Could not set socket's blocking mode: {}", err), loc};
  }
#  endif // _WIN32
}
#endif // defined(_WIN32) || __has_include(<fcntl.h>)


PQXX_COLD void
pqxx::connection::set_verbosity(error_verbosity verbosity) & noexcept
{
  PQsetErrorVerbosity(m_conn, static_cast<PGVerbosity>(verbosity));
}


namespace
{
/// Unique pointer to PGnotify.
using notify_ptr = std::unique_ptr<PGnotify, void (*)(void const *)>;


/// Get one notification from a connection, or null.
notify_ptr get_notif(pqxx::internal::pq::PGconn *cx)
{
  return {PQnotifies(cx), pqxx::internal::pq::pqfreemem};
}
} // namespace


int pqxx::connection::get_notifs(sl loc)
{
  if (not consume_input())
    throw broken_connection{"Connection lost.", loc};

  // Even if somehow we receive notifications during our transaction, don't
  // deliver them.
  if (m_trans != nullptr) [[unlikely]]
    return 0;

  int notifs = 0;

  // Old mechanism.  This is going away.
  for (auto N{get_notif(m_conn)}; N.get(); N = get_notif(m_conn))
  {
    notifs++;

    std::string const channel{N->relname};

    auto const Hit{m_receivers.equal_range(channel)};
    if (Hit.second != Hit.first)
    {
      std::string const payload{N->extra};
      for (auto i{Hit.first}; i != Hit.second; ++i) try
        {
          (*i->second)(payload, N->be_pid);
        }
        catch (std::exception const &e)
        {
          try
          {
            process_notice(std::format(
              "Exception in notification receiver '{}': {}\n", i->first,
              e.what()));
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
    }

    auto const handler{m_notification_handlers.find(N->relname)};
    if (handler != std::end(m_notification_handlers))
      (handler->second)(notification{
        .conn = *this,
        .channel = channel,
        .payload = N->extra,
        .backend_pid = N->be_pid,
      });

    N.reset();
  }

  return notifs;
}


PQXX_COLD char const *pqxx::connection::dbname() const noexcept
{
  return PQdb(m_conn);
}


PQXX_COLD char const *pqxx::connection::username() const noexcept
{
  return PQuser(m_conn);
}


PQXX_COLD char const *pqxx::connection::hostname() const noexcept
{
  return PQhost(m_conn);
}


PQXX_COLD char const *pqxx::connection::port() const noexcept
{
  return PQport(m_conn);
}


PQXX_COLD std::optional<int> pqxx::connection::port_number(sl loc) const
{
  char const *const ptr{PQport(m_conn)};
  if ((ptr == nullptr) or (*ptr == '\0'))
    return {};
  else
    return from_string<int>(
      ptr, conversion_context{encoding_group::ascii_safe, loc});
}


char const *pqxx::connection::err_msg() const noexcept
{
  return (m_conn == nullptr) ? "No connection to database" :
                               PQerrorMessage(m_conn);
}


PQXX_COLD void pqxx::connection::register_errorhandler(errorhandler *handler)
{
  m_notice_waiters->errorhandlers.push_back(handler);
}


PQXX_COLD void
pqxx::connection::unregister_errorhandler(errorhandler *handler) noexcept
{
  // The errorhandler itself will take care of nulling its pointer to this
  // connection.
  m_notice_waiters->errorhandlers.remove(handler);
}


PQXX_COLD std::vector<pqxx::errorhandler *>
pqxx::connection::get_errorhandlers() const
{
  return {
    std::begin(m_notice_waiters->errorhandlers),
    std::end(m_notice_waiters->errorhandlers)};
}


pqxx::result
pqxx::connection::exec(std::string_view query, std::string_view desc, sl loc)
{
  return exec(std::make_shared<std::string>(query), desc, loc);
}


pqxx::result pqxx::connection::exec(
  std::shared_ptr<std::string> const &query, std::string_view desc, sl loc)
{
  auto res{make_result(PQexec(m_conn, query->c_str()), query, desc, loc)};
  get_notifs(loc);
  return res;
}


std::string pqxx::connection::encrypt_password(
  char const user[], char const password[], char const *algorithm)
{
  auto const buf{PQencryptPasswordConn(m_conn, password, user, algorithm)};
  std::unique_ptr<char const[], void (*)(void const *)> const ptr{
    buf, pqxx::internal::pq::pqfreemem};
  return (ptr.get());
}


void pqxx::connection::prepare(
  char const name[], char const definition[], sl loc) &
{
  auto const q{
    std::make_shared<std::string>(std::format("[PREPARE {}]", name))};

  auto const r{
    make_result(PQprepare(m_conn, name, definition, 0, nullptr), q, *q, loc)};
}


void pqxx::connection::prepare(char const definition[], sl loc) &
{
  this->prepare("", definition, loc);
}


void pqxx::connection::unprepare(std::string_view name, sl loc)
{
  exec(std::format("DEALLOCATE {}", quote_name(name)), loc);
}


pqxx::result pqxx::connection::exec_prepared(
  std::string_view statement, internal::c_params const &args, sl loc)
{
  auto const q{std::make_shared<std::string>(statement)};
  auto const pq_result{PQexecPrepared(
    m_conn, q->c_str(),
    check_cast<int>(std::size(args.values), "exec_prepared"sv, loc),
    args.values.data(), args.lengths.data(),
    reinterpret_cast<int const *>(args.formats.data()),
    static_cast<int>(format::text))};
  auto r{make_result(pq_result, q, statement, loc)};
  get_notifs(loc);
  return r;
}


void pqxx::connection::close(sl)
{
  // Just in case PQfinish() doesn't handle nullptr nicely.
  if (m_conn == nullptr)
    return;
  try
  {
    if (m_trans) [[unlikely]]
      process_notice(std::format(
        "Closing connection while {} is still open.\n",
        internal::describe_object("transaction"sv, m_trans->name())));

    if (not std::empty(m_receivers))
    {
      [[unlikely]] process_notice(
        "Closing connection with outstanding receivers.\n");
      m_receivers.clear();
    }

    if (m_notice_waiters)
    {
      // It's a bit iffy to unregister these in this destructor.  There may
      // still be result objects that want to process notices.  But it's an
      // improvement over the 7.9-and-older situation where you'd simply get a
      // stale pointer.  Better yet, this whole mechanism is going away.
#include "pqxx/internal/ignore-deprecated-pre.hxx"
      auto old_handlers{get_errorhandlers()};
#include "pqxx/internal/ignore-deprecated-post.hxx"
      auto const rbegin{std::crbegin(old_handlers)},
        rend{std::crend(old_handlers)};
      for (auto i{rbegin}; i != rend; ++i)
        pqxx::internal::gate::errorhandler_connection{**i}.unregister();
    }

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


namespace
{
/// Return a name for t, if t is non-null and has a name; or empty string.
std::string_view get_name(pqxx::transaction_base const *t)
{
  return (t == nullptr) ? ""sv : t->name();
}
} // namespace


void pqxx::connection::register_transaction(transaction_base *t)
{
  internal::check_unique_register(
    m_trans, "transaction", get_name(m_trans), t, "transaction", get_name(t));
  m_trans = t;
}


void pqxx::connection::unregister_transaction(transaction_base *t) noexcept
{
  try
  {
    internal::check_unique_unregister(
      m_trans, "transaction", get_name(m_trans), t, "transaction",
      get_name(t));
  }
  catch (std::exception const &e)
  {
    // TODO: Make at least an attempt to append a newline.
    process_notice(e.what());
  }
  m_trans = nullptr;
}


std::pair<std::unique_ptr<char[], void (*)(void const *)>, std::size_t>
pqxx::connection::read_copy_line(sl loc)
{
  char *buf{nullptr};

  // Allocate once, re-use across invocations.
  static auto const q{std::make_shared<std::string>("[END COPY]")};

  auto const line_len{PQgetCopyData(m_conn, &buf, false)};
  switch (line_len)
  {
  case -2: // Error.
    throw failure{
      std::format("Reading of table data failed: {}", err_msg()), loc};

  case -1: // End of COPY.
    make_result(PQgetResult(m_conn), q, *q, loc);
    return std::make_pair(
      std::unique_ptr<char[], void (*)(void const *)>{
        nullptr, pqxx::internal::pq::pqfreemem},
      0u);

  case 0: // "Come back later."
    throw internal_error{"table read inexplicably went asynchronous", loc};

  default: // Success, got buffer size.
    [[likely]]
    {
      // Line size includes a trailing zero, which we ignore.
      auto const text_len{static_cast<std::size_t>(line_len) - 1};
      return std::make_pair(
        std::unique_ptr<char[], void (*)(void const *)>{
          buf, pqxx::internal::pq::pqfreemem},
        text_len);
    }
  }
}


void pqxx::connection::write_copy_line(std::string_view line, sl loc)
{
  static std::string const err_prefix{"Error writing to table: "};
  auto const size{check_cast<int>(
    std::ssize(line), "Line in stream_to is too long to process."sv, loc)};
  if (PQputCopyData(m_conn, line.data(), size) <= 0) [[unlikely]]
    throw failure{err_prefix + err_msg(), loc};
  if (PQputCopyData(m_conn, "\n", 1) <= 0) [[unlikely]]
    throw failure{err_prefix + err_msg(), loc};
}


void pqxx::connection::end_copy_write(sl loc)
{
  int const res{PQputCopyEnd(m_conn, nullptr)};
  switch (res)
  {
  case -1:
    throw failure{std::format("Write to table failed: {}", err_msg()), loc};
  case 0:
    throw internal_error{"table write is inexplicably asynchronous", loc};
  case 1:
    // Normal termination.  Retrieve result object.
    break;

  default:
    throw internal_error{
      std::format("unexpected result {} from PQputCopyEnd()", res), loc};
  }

  static auto const q{std::make_shared<std::string>("[END COPY]")};
  make_result(PQgetResult(m_conn), q, *q, loc);
}


void pqxx::connection::start_exec(char const query[])
{
  if (PQsendQuery(m_conn, query) == 0) [[unlikely]]
    throw failure{err_msg()};
}


pqxx::internal::pq::PGresult *pqxx::connection::get_result()
{
  return PQgetResult(m_conn);
}


std::size_t pqxx::connection::esc_to_buf(
  std::string_view text, std::span<char> buf, sl loc) const
{
  int err{0};
  auto const copied{PQescapeStringConn(
    m_conn, std::data(buf), std::data(text), std::size(text), &err)};
  if (err) [[unlikely]]
    throw argument_error{err_msg(), loc};
  return copied;
}


std::string pqxx::connection::esc(std::string_view text, sl loc) const
{
  std::string buf;
  buf.resize(2 * std::size(text) + 1);
  auto const copied{esc_to_buf(text, buf, loc)};
  buf.resize(copied);
  return buf;
}


std::string pqxx::connection::esc_raw(bytes_view bin) const
{
  return pqxx::internal::esc_bin(bin);
}


std::string pqxx::connection::quote_raw(bytes_view bytes) const
{
  return std::format("'{}'::bytea", esc_raw(bytes));
}


std::string pqxx::connection::quote_name(std::string_view identifier) const
{
  std::unique_ptr<char[], void (*)(void const *)> const buf{
    PQescapeIdentifier(m_conn, identifier.data(), std::size(identifier)),
    pqxx::internal::pq::pqfreemem};
  if (buf == nullptr) [[unlikely]]
    throw failure{err_msg()};
  return std::string{buf.get()};
}


std::string pqxx::connection::quote_table(std::string_view table_name) const
{
  return this->quote_name(table_name);
}


std::string pqxx::connection::quote_table(table_path path) const
{
  return separated_list(
    ".", std::begin(path), std::end(path),
    [this](auto name) { return this->quote_name(*name); });
}


std::string pqxx::connection::esc_like(
  std::string_view text, char escape_char, sl loc) const
{
  auto const sz{std::size(text)};
  if (sz == 0u)
    return "";
  auto const data{std::data(text)};
  auto const finder{
    pqxx::internal::get_char_finder<'_', '%'>(get_encoding_group(loc), loc)};

  // We're going to loop in a counterintuitive order.  First we look for the
  // end of the leading sequence of unremarkable characters.
  auto next{finder(text, 0, loc)};
  if (next >= sz)
    // No special characters at all.  Copy the entire input.
    return std::string{text};

  // Still here?  Then we'll need to do some actual escaping.  Start a buffer.
  //
  // In the worst case we'll need 2 * sz bytes.  So why not just reserve those?
  // Because if the string is short, reserving too much space may lose us the
  // Short String Optimization and cost us an unnecessary memory allocation.
  //
  // For large inputs, std::string will have an efficient growth algorithm so
  // it's not like we'll be re-allocating for every byte.
  std::string out;

  // Append any leading unremarkable characters that we just scanned.
  out.append(data, data + next);

  // Now we loop.  This is the counterintuitive part.  We'll be dealing in
  // chunks of text, each _starting_ with a special character with zero or more
  // unremarkable ones after.
  for (std::size_t here{next}; here < sz; here = next)
  {
    // So we found a special character.  It's exactly 1 byte long.
    assert((data[here] == '%') or (data[here] == '_'));

    // Look for the next stopping point after that, whether another special
    // character or the end of the input.  (This search may even start at the
    // end of the input, and find nothing at all.)
    next = finder(text, here + 1, loc);

    // Now we start appending: an escape character, followed by the special
    // character (so that it gets escaped) and its tail of unremarkable
    // characters after that.  This way we copy the special character and the
    // unremarkable ones all in one go.
    out.push_back(escape_char);
    out.append(data + here, data + next);
  }
  return out;
}


int pqxx::connection::await_notification(sl loc)
{
  int notifs = get_notifs(loc);
  if (notifs == 0)
  {
    [[likely]] internal::wait_fd(socket_of(m_conn), true, false, 10, 0);
    notifs = get_notifs(loc);
  }
  return notifs;
}


int pqxx::connection::await_notification(
  std::time_t seconds, long microseconds, sl loc)
{
  int const notifs = get_notifs(loc);
  if (notifs == 0)
  {
    [[likely]] internal::wait_fd(
      socket_of(m_conn), true, false,
      check_cast<unsigned>(seconds, "Seconds out of range.", loc),
      check_cast<unsigned>(microseconds, "Microseconds out of range.", loc));
    return get_notifs(loc);
  }
  return notifs;
}


std::string pqxx::connection::adorn_name(std::string_view n)
{
  auto const id{to_string(++m_unique_id)};
  if (std::empty(n))
    return std::format("x{}", id);
  else
    return std::format("{}_{}", n, id);
}


std::string pqxx::connection::get_client_encoding(sl loc) const
{
  return internal::name_encoding(encoding_id(loc));
}


PQXX_ZARGS PQXX_COLD void
pqxx::connection::set_client_encoding(char const encoding[], sl loc) &
{
  if ((encoding == nullptr) or (encoding[0] == '\0'))
    throw pqxx::argument_error{
      "No encoding string passed to set_client_encoding.", loc};

  // TODO: Remove this check once JOHAB is gone from all supported versions.
  if (
    ((encoding[0] == 'J') or (encoding[0] == 'j')) and
    ((encoding[1] == 'O') or (encoding[1] == 'o')) and
    ((encoding[2] == 'H') or (encoding[2] == 'h')) and
    ((encoding[3] == 'A') or (encoding[3] == 'a')) and
    ((encoding[4] == 'B') or (encoding[4] == 'b')) and (encoding[5] == '\0'))
  {
    throw pqxx::argument_error{"JOHAB encoding is not supported.", loc};
  }

  switch (auto const retval{PQsetClientEncoding(m_conn, encoding)}; retval)
  {
  case 0:
    // OK.
    [[likely]] break;
  case -1:
    if (is_open())
      throw failure{
        std::format("Setting client encoding failed ('{}').", encoding), loc};
    else
      throw broken_connection{"Lost connection to the database server.", loc};
  default:
    throw internal_error{
      std::format("Unexpected result from PQsetClientEncoding: {}", retval),
      loc};
  }
}


int pqxx::connection::encoding_id(sl loc) const
{
  int const enc{PQclientEncoding(m_conn)};
  if (enc == -1)
  {
    // PQclientEncoding does not query the database, but it does check for
    // broken connections.  And unfortunately, we check the encoding right
    // *before* checking a query result for failure.  So, we need to handle
    // connection failure here and it will apply in lots of places.
    // TODO: Make pqxx::result::result(...) do all the checking.
    [[unlikely]]
    if (is_open())
      throw failure{"Could not obtain client encoding.", loc};
    else
      throw broken_connection{"Lost connection to the database server.", loc};
  }
  [[likely]] return enc;
}


pqxx::result pqxx::connection::exec_params(
  std::string_view query, internal::c_params const &args, sl loc)
{
  auto const q{std::make_shared<std::string>(query)};
  auto const pq_result{PQexecParams(
    m_conn, q->c_str(),
    check_cast<int>(std::size(args.values), "exec_params"sv, loc), nullptr,
    args.values.data(), args.lengths.data(),
    reinterpret_cast<int const *>(args.formats.data()),
    static_cast<int>(format::text))};
  auto r{make_result(pq_result, q, loc)};
  get_notifs(loc);
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
#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
  char const *var{std::getenv(opt.envvar)};
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif
  if (var == nullptr)
  {
    // There's an environment variable for this setting, but it's not set.
    return opt.compiled;
  }

  // The environment variable is the prevailing default.
  return var;
}


/// Wrapper for `PQconninfoFree()`, with C++ linkage.
void pqconninfofree(PQconninfoOption *ptr)
{
  PQconninfoFree(ptr);
}


/// Quote and escape a connection string parameter as needed.
/** There's no encoding support here because all of that is based on client
 * encodings.  And there's no such thing when we're not connected yet
 */
std::string quote_connect_param(std::string_view val)
{
  std::string buf;
  buf.reserve(2 * std::size(val) + 2);

  bool const quote{
    pqxx::str_contains(val, ' ') or pqxx::str_contains(val, '\'')};

  if (quote)
    buf += '\'';
  for (auto c : val)
  {
    if ((c == '\\') or (c == '\''))
      buf += '\\';
    buf += c;
  }
  if (quote)
    buf += '\'';
  return buf;
}
} // namespace


std::string pqxx::connection::connection_string() const
{
  if (m_conn == nullptr) [[unlikely]]
    throw usage_error{"Can't get connection string: connection is not open."};

  std::unique_ptr<PQconninfoOption[], void (*)(PQconninfoOption *)> const
    parms{PQconninfo(m_conn), pqconninfofree};
  if (parms == nullptr)
    throw std::bad_alloc{};

  std::string buf;
  for (std::size_t i{0}; parms.get()[i].keyword != nullptr; ++i)
  {
    auto const param{parms.get()[i]};
    if (param.val != nullptr)
    {
      auto const default_val{get_default(param)};
      // Skip over parameters that have the default value.
      if (
        (default_val == nullptr) or (std::strcmp(param.val, default_val) != 0))
      {
        if (not std::empty(buf))
          buf.push_back(' ');
        buf += param.keyword;
        buf.push_back('=');
        // There is no particular encoding support for connection strings in
        // libpq, not even percent-encoding.  They just have to be in ASCII or
        // some ASCII-safe encoding.
        buf += quote_connect_param(param.val);
      }
    }
  }
  return buf;
}


#if defined(_WIN32) || __has_include(<fcntl.h>)
pqxx::connecting::connecting(zview connection_string, sl loc) :
        m_conn{connection::connect_nonblocking, connection_string, loc}
{}
#endif // defined(_WIN32) || __has_include(<fcntl.h>


#if defined(_WIN32) || __has_include(<fcntl.h>)
void pqxx::connecting::process(sl loc) &
{
  auto const [reading, writing]{m_conn.poll_connect(loc)};
  m_reading = reading;
  m_writing = writing;
}
#endif // defined(_WIN32) || __has_include(<fcntl.h>


#if defined(_WIN32) || __has_include(<fcntl.h>)
pqxx::connection pqxx::connecting::produce(sl loc) &&
{
  if (!done())
    throw usage_error{
      "Tried to produce a nonblocking connection before it was done.", loc};
  m_conn.complete_connection(loc);
  return {std::move(m_conn), loc};
}
#endif // defined(_WIN32) || __has_include(<fcntl.h>)
