/** Implementation of the pqxx::connection_base abstract base class.
 *
 * pqxx::connection_base encapsulates a frontend to backend connection.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iterator>
#include <memory>
#include <stdexcept>

#if defined(_WIN32)
// Includes for WSAPoll().
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mstcpip.h>
#elif defined(HAVE_POLL)
// Include for poll().
#include <poll.h>
#elif defined(HAVE_SYS_SELECT_H)
// Include for select() on (recent) POSIX systems.
#include <sys/select.h>
#else
// Includes for select() according to various older standards.
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#endif
#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#include "libpq-fe.h"

#include "pqxx/binarystring"
#include "pqxx/connection"
#include "pqxx/connection_base"
#include "pqxx/nontransaction"
#include "pqxx/pipeline"
#include "pqxx/result"
#include "pqxx/strconv"
#include "pqxx/transaction"
#include "pqxx/notification"

#include "pqxx/internal/gates/connection-reactivation_avoidance_exemption.hxx"
#include "pqxx/internal/gates/errorhandler-connection.hxx"
#include "pqxx/internal/gates/result-creation.hxx"
#include "pqxx/internal/gates/result-connection.hxx"

using namespace pqxx;
using namespace pqxx::internal;
using namespace pqxx::prepare;


namespace
{
extern "C"
{
// The PQnoticeProcessor that receives an error or warning from libpq and sends
// it to the appropriate connection for processing.
void pqxx_notice_processor(void *conn, const char *msg)
{
  reinterpret_cast<pqxx::connection_base *>(conn)->process_notice(msg);
}
} // extern "C"
} // namespace


std::string pqxx::encrypt_password(
        const std::string &user, const std::string &password)
{
  std::unique_ptr<char, void (*)(char *)> p(
	PQencryptPassword(password.c_str(), user.c_str()),
        freepqmem_templated<char>);
  return std::string(p.get());
}


void pqxx::connection_base::init()
{
  m_conn = m_policy.do_startconnect(m_conn);
  if (m_policy.is_ready(m_conn)) activate();
}


pqxx::result pqxx::connection_base::make_result(
	internal::pq::PGresult *rhs,
	const std::string &query)
{
  return gate::result_creation::create(rhs, query);
}


int pqxx::connection_base::backendpid() const noexcept
{
  return m_conn ? PQbackendPID(m_conn) : 0;
}


namespace
{
PQXX_PURE int socket_of(const ::pqxx::internal::pq::PGconn *c) noexcept
{
  return c ? PQsocket(c) : -1;
}
}


int pqxx::connection_base::sock() const noexcept
{
  return socket_of(m_conn);
}


void pqxx::connection_base::activate()
{
  if (!is_open())
  {
    if (m_inhibit_reactivation)
      throw broken_connection(
	"Could not reactivate connection; "
	"reactivation is inhibited");

    // If any objects were open that didn't survive the closing of our
    // connection, don't try to reactivate
    if (m_reactivation_avoidance.get()) return;

    try
    {
      m_conn = m_policy.do_startconnect(m_conn);
      m_conn = m_policy.do_completeconnect(m_conn);
      m_completed = true;	// (But retracted if error is thrown below)

      if (!is_open()) throw broken_connection();

      set_up_state();
    }
    catch (const broken_connection &e)
    {
      disconnect();
      m_completed = false;
      throw broken_connection(e.what());
    }
    catch (const std::exception &)
    {
      m_completed = false;
      throw;
    }
  }
}


void pqxx::connection_base::deactivate()
{
  if (!m_conn) return;

  if (m_trans.get())
    throw usage_error(
	"Attempt to deactivate connection while " +
	m_trans.get()->description() + " still open");

  if (m_reactivation_avoidance.get())
  {
    process_notice(
	"Attempt to deactivate connection while it is in a state "
	"that cannot be fully recovered later (ignoring)");
    return;
  }

  m_completed = false;
  m_conn = m_policy.do_disconnect(m_conn);
}


void pqxx::connection_base::simulate_failure()
{
  if (m_conn)
  {
    m_conn = m_policy.do_disconnect(m_conn);
    inhibit_reactivation(true);
  }
}


int pqxx::connection_base::protocol_version() const noexcept
{
  return m_conn ? PQprotocolVersion(m_conn) : 0;
}


int pqxx::connection_base::server_version() const noexcept
{
  return m_serverversion;
}


void pqxx::connection_base::set_variable(const std::string &Var,
	const std::string &Value)
{
  if (m_trans.get())
  {
    // We're in a transaction.  The variable should go in there.
    m_trans.get()->set_variable(Var, Value);
  }
  else
  {
    // We're not in a transaction.  Set a session variable.
    if (is_open()) raw_set_var(Var, Value);
    m_vars[Var] = Value;
  }
}


std::string pqxx::connection_base::get_variable(const std::string &Var)
{
  return m_trans.get() ? m_trans.get()->get_variable(Var) : raw_get_var(Var);
}


std::string pqxx::connection_base::raw_get_var(const std::string &Var)
{
  // Is this variable in our local map of set variables?
  const auto i = m_vars.find(Var);
  if (i != m_vars.end()) return i->second;

  return exec(("SHOW " + Var).c_str(), 0).at(0).at(0).as(std::string());
}


void pqxx::connection_base::clearcaps() noexcept
{
  m_caps.reset();
}


/** Set up various parts of logical connection state that may need to be
 * recovered because the physical connection to the database was lost and is
 * being reset, or that may not have been initialized yet.
 */
void pqxx::connection_base::set_up_state()
{
  if (!m_conn)
    throw internal_error("set_up_state() on no connection");

  if (status() != CONNECTION_OK)
  {
    const auto msg = err_msg();
    m_conn = m_policy.do_disconnect(m_conn);
    throw failure(msg);
  }

  read_capabilities();

  for (auto &p: m_prepared) p.second.registered = false;

  PQsetNoticeProcessor(m_conn, pqxx_notice_processor, this);

  internal_set_trace();

  if (!m_receivers.empty() || !m_vars.empty())
  {
    std::stringstream restore_query;

    // Pipeline all queries needed to restore receivers and variables, so we can
    // send them over in one go.

    // Reinstate all active receivers
    if (!m_receivers.empty())
    {
      std::string Last;
      for (auto &i: m_receivers)
      {
        // m_receivers can handle multiple receivers waiting on the same event;
        // issue just one LISTEN for each event.
        if (i.first != Last)
        {
          restore_query << "LISTEN \"" << i.first << "\"; ";
          Last = i.first;
        }
      }
    }

    for (auto &i: m_vars)
      restore_query << "SET " << i.first << "=" << i.second << "; ";

    // Now do the whole batch at once
    PQsendQuery(m_conn, restore_query.str().c_str());
    result r;
    do
      r = make_result(PQgetResult(m_conn), "[RECONNECT]");
    while (gate::result_connection(r));
  }

  m_completed = true;
  if (!is_open()) throw broken_connection();
}


void pqxx::connection_base::check_result(const result &R)
{
  if (!is_open()) throw broken_connection();

  // A shame we can't quite detect out-of-memory to turn this into a bad_alloc!
  if (!gate::result_connection(R)) throw failure(err_msg());

  gate::result_creation(R).check_status();
}


void pqxx::connection_base::disconnect() noexcept
{
  // When we activate again, the server may be different!
  clearcaps();

  m_conn = m_policy.do_disconnect(m_conn);
}


bool pqxx::connection_base::is_open() const noexcept
{
  return m_conn && m_completed && (status() == CONNECTION_OK);
}


void pqxx::connection_base::process_notice_raw(const char msg[]) noexcept
{
  if (!msg || !*msg) return;
  const auto
	rbegin = m_errorhandlers.rbegin(),
	rend = m_errorhandlers.rend();
  for (auto i = rbegin; i != rend && (**i)(msg); ++i) ;
}


void pqxx::connection_base::process_notice(const char msg[]) noexcept
{
  if (!msg) return;
  const auto len = strlen(msg);
  if (len == 0) return;
  if (msg[len-1] == '\n')
  {
    process_notice_raw(msg);
  }
  else try
  {
    // Newline is missing.  Try the C++ string version of this function.
    process_notice(std::string(msg));
  }
  catch (const std::exception &)
  {
    // If we can't even do that, use plain old buffer copying instead
    // (unavoidably, this will break up overly long messages!)
    const char separator[] = "[...]\n";
    char buf[1007];
    size_t bytes = sizeof(buf)-sizeof(separator)-1;
    size_t written;
    strcpy(&buf[bytes], separator);
    // Write all chunks but last.  Each will fill the buffer exactly.
    for (written = 0; (written+bytes) < len; written += bytes)
    {
      memcpy(buf, &msg[written], bytes);
      process_notice_raw(buf);
    }
    // Write any remaining bytes (which won't fill an entire buffer)
    bytes = len-written;
    memcpy(buf, &msg[written], bytes);
    // Add trailing nul byte, plus newline unless there already is one
    strcpy(&buf[bytes], &"\n"[buf[bytes-1]=='\n']);
    process_notice_raw(buf);
  }
}


void pqxx::connection_base::process_notice(const std::string &msg) noexcept
{
  // Ensure that message passed to errorhandler ends in newline
  if (msg[msg.size()-1] == '\n')
  {
    process_notice_raw(msg.c_str());
  }
  else try
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


void pqxx::connection_base::trace(FILE *Out) noexcept
{
  m_trace = Out;
  if (m_conn) internal_set_trace();
}


void pqxx::connection_base::add_receiver(pqxx::notification_receiver *T)
{
  if (!T) throw argument_error("Null receiver registered");

  // Add to receiver list and attempt to start listening.
  const auto p = m_receivers.find(T->channel());
  const receiver_list::value_type NewVal(T->channel(), T);

  if (p == m_receivers.end())
  {
    // Not listening on this event yet, start doing so.
    const std::string LQ("LISTEN \"" + T->channel() + "\"");

    if (is_open()) try
    {
      check_result(make_result(PQexec(m_conn, LQ.c_str()), LQ));
    }
    catch (const broken_connection &)
    {
    }
    m_receivers.insert(NewVal);
  }
  else
  {
    m_receivers.insert(p, NewVal);
  }
}


void pqxx::connection_base::remove_receiver(pqxx::notification_receiver *T)
	noexcept
{
  if (!T) return;

  try
  {
    const std::pair<const std::string, notification_receiver *> needle(
	T->channel(), T);
    auto R = m_receivers.equal_range(needle.first);
    const auto i = find(R.first, R.second, needle);

    if (i == R.second)
    {
      process_notice(
	"Attempt to remove unknown receiver '" + needle.first + "'");
    }
    else
    {
      // Erase first; otherwise a notification for the same receiver may yet
      // come in and wreak havoc.  Thanks Dragan Milenkovic.
      const bool gone = (m_conn && (R.second == ++R.first));
      m_receivers.erase(i);
      if (gone) exec(("UNLISTEN \"" + needle.first + "\"").c_str(), 0);
    }
  }
  catch (const std::exception &e)
  {
    process_notice(e.what());
  }
}


bool pqxx::connection_base::consume_input() noexcept
{
  return PQconsumeInput(m_conn) != 0;
}


bool pqxx::connection_base::is_busy() const noexcept
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
  explicit cancel_wrapper(PGconn *conn) :
    m_cancel(nullptr),
    m_errbuf()
  {
    if (conn)
    {
      m_cancel = PQgetCancel(conn);
      if (!m_cancel) throw std::bad_alloc();
    }
  }
  ~cancel_wrapper() { if (m_cancel) PQfreeCancel(m_cancel); }

  void operator()()
  {
    if (m_cancel && !PQcancel(m_cancel, m_errbuf, int(sizeof(m_errbuf))))
      throw sql_error(std::string(m_errbuf));
  }
};
}


void pqxx::connection_base::cancel_query()
{
  cancel_wrapper cancel(m_conn);
  cancel();
}


void pqxx::connection_base::set_verbosity(error_verbosity verbosity) noexcept
{
    PQsetErrorVerbosity(m_conn, static_cast<PGVerbosity>(verbosity));
    m_verbosity = verbosity;
}


namespace
{
/// Unique pointer to PGnotify.
using notify_ptr = std::unique_ptr<PGnotify, void (*)(PGnotify *)>;


/// Get one notification from a connection, or null.
notify_ptr get_notif(pqxx::internal::pq::PGconn *conn)
{
  return notify_ptr(PQnotifies(conn), freepqmem_templated<PGnotify>);
}
}


int pqxx::connection_base::get_notifs()
{
  if (!is_open()) return 0;

  if (!consume_input()) throw broken_connection();

  // Even if somehow we receive notifications during our transaction, don't
  // deliver them.
  if (m_trans.get()) return 0;

  int notifs = 0;
  for (auto N = get_notif(m_conn); N.get(); N = get_notif(m_conn))
  {
    notifs++;

    const auto Hit = m_receivers.equal_range(std::string(N->relname));
    for (auto i = Hit.first; i != Hit.second; ++i) try
    {
      (*i->second)(N->extra, N->be_pid);
    }
    catch (const std::exception &e)
    {
      try
      {
        process_notice(
		"Exception in notification receiver '" +
		i->first +
		"': " +
		e.what() +
		"\n");
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


const char *pqxx::connection_base::dbname()
{
  if (!m_conn) activate();
  return PQdb(m_conn);
}


const char *pqxx::connection_base::username()
{
  if (!m_conn) activate();
  return PQuser(m_conn);
}


const char *pqxx::connection_base::hostname()
{
  if (!m_conn) activate();
  return PQhost(m_conn);
}


const char *pqxx::connection_base::port()
{
  if (!m_conn) activate();
  return PQport(m_conn);
}


const char *pqxx::connection_base::err_msg() const noexcept
{
  return m_conn ? PQerrorMessage(m_conn) : "No connection to database";
}


void pqxx::connection_base::register_errorhandler(errorhandler *handler)
{
  m_errorhandlers.push_back(handler);
}


void pqxx::connection_base::unregister_errorhandler(errorhandler *handler)
  noexcept
{
  // The errorhandler itself will take care of nulling its pointer to this
  // connection.
  m_errorhandlers.remove(handler);
}


std::vector<errorhandler *> pqxx::connection_base::get_errorhandlers() const
{
  return std::vector<errorhandler *>(
    std::begin(m_errorhandlers), std::end(m_errorhandlers));
}


pqxx::result pqxx::connection_base::exec(const char Query[], int Retries)
{
  activate();

  auto R = make_result(PQexec(m_conn, Query), Query);

  while ((Retries > 0) && !gate::result_connection(R) && !is_open())
  {
    Retries--;
    reset();
    if (is_open()) R = make_result(PQexec(m_conn, Query), Query);
  }

  check_result(R);

  get_notifs();
  return R;
}


void pqxx::connection_base::prepare(
	const std::string &name,
	const std::string &definition)
{
  auto i = m_prepared.find(name);
  if (i != m_prepared.end())
  {
    if (definition != i->second.definition)
    {
      if (!name.empty())
        throw argument_error(
		"Inconsistent redefinition of prepared statement " + name);

      i->second.registered = false;
      i->second.definition = definition;
    }
  }
  else
  {
    m_prepared.insert(make_pair(
	name,
	prepare::internal::prepared_def(definition)));
  }
}


void pqxx::connection_base::prepare(const std::string &definition)
{
  this->prepare(std::string(), definition);
}


void pqxx::connection_base::unprepare(const std::string &name)
{
  auto i = m_prepared.find(name);

  // Quietly ignore duplicated or spurious unprepare()s
  if (i == m_prepared.end()) return;

  if (i->second.registered) exec(("DEALLOCATE \"" + name + "\"").c_str(), 0);

  m_prepared.erase(i);
}


pqxx::prepare::internal::prepared_def &
pqxx::connection_base::find_prepared(const std::string &statement)
{
  auto s = m_prepared.find(statement);
  if (s == m_prepared.end())
    throw argument_error("Unknown prepared statement '" + statement + "'");
  return s->second;
}


pqxx::prepare::internal::prepared_def &
pqxx::connection_base::register_prepared(const std::string &name)
{
  activate();
  auto &s = find_prepared(name);

  // "Register" (i.e., define) prepared statement with backend on demand
  if (!s.registered)
  {
    auto r = make_result(
      PQprepare(m_conn, name.c_str(), s.definition.c_str(), 0, nullptr),
      "[PREPARE " + name + "]");
    check_result(r);
    s.registered = !name.empty();
    return s;
  }

  return s;
}


void pqxx::connection_base::prepare_now(const std::string &name)
{
  register_prepared(name);
}


pqxx::result pqxx::connection_base::prepared_exec(
	const std::string &statement,
	const char *const params[],
	const int paramlengths[],
	const int binary[],
	int nparams)
{
  register_prepared(statement);
  activate();
  auto r = make_result(
	PQexecPrepared(
		m_conn,
		statement.c_str(),
		nparams,
		params,
		paramlengths,
		binary,
		0),
    	statement);
  check_result(r);
  get_notifs();
  return r;
}


pqxx::result pqxx::connection_base::exec_prepared(
	const std::string &statement,
	const internal::params &args)
{
  register_prepared(statement);
  activate();
  const auto pointers = args.get_pointers();
  const auto pq_result = PQexecPrepared(
	m_conn,
	statement.c_str(),
	int(args.nonnulls.size()),
	&pointers[0],
	&args.lengths[0],
	&args.binaries[0],
	0);
  const auto r = make_result(pq_result, statement);
  check_result(r);
  get_notifs();
  return r;
}


bool pqxx::connection_base::prepared_exists(const std::string &statement) const
{
  auto s = m_prepared.find(statement);
  return s != PSMap::const_iterator(m_prepared.end());
}


void pqxx::connection_base::reset()
{
  if (m_inhibit_reactivation)
    throw broken_connection(
	"Could not reset connection: reactivation is inhibited");
  if (m_reactivation_avoidance.get()) return;

  // TODO: Probably need to go through a full disconnect/reconnect!
  // Forget about any previously ongoing connection attempts
  m_conn = m_policy.do_dropconnect(m_conn);
  m_completed = false;

  if (m_conn)
  {
    // Reset existing connection
    PQreset(m_conn);
    set_up_state();
  }
  else
  {
    // No existing connection--start a new one
    activate();
  }
}


void pqxx::connection_base::close() noexcept
{
  m_completed = false;
  inhibit_reactivation(false);
  m_reactivation_avoidance.clear();
  try
  {
    if (m_trans.get())
      process_notice(
	"Closing connection while " + m_trans.get()->description() +
	" still open");

    if (!m_receivers.empty())
    {
      process_notice("Closing connection with outstanding receivers.");
      m_receivers.clear();
    }

    PQsetNoticeProcessor(m_conn, nullptr, nullptr);
    std::list<errorhandler *> old_handlers;
    m_errorhandlers.swap(old_handlers);
    const auto
	rbegin = old_handlers.rbegin(),
	rend = old_handlers.rend();
    for (auto i = rbegin; i!=rend; ++i)
      gate::errorhandler_connection_base(**i).unregister();

    m_conn = m_policy.do_disconnect(m_conn);
  }
  catch (...)
  {
  }
}


void pqxx::connection_base::raw_set_var(
	const std::string &Var,
	const std::string &Value)
{
    exec(("SET " + Var + "=" + Value).c_str(), 0);
}


void pqxx::connection_base::add_variables(
	const std::map<std::string,std::string> &Vars)
{
  for (auto &i: Vars) m_vars[i.first] = i.second;
}


void pqxx::connection_base::internal_set_trace() noexcept
{
  if (m_conn)
  {
    if (m_trace) PQtrace(m_conn, m_trace);
    else PQuntrace(m_conn);
  }
}


int pqxx::connection_base::status() const noexcept
{
  return PQstatus(m_conn);
}


void pqxx::connection_base::register_transaction(transaction_base *T)
{
  m_trans.register_guest(T);
}


void pqxx::connection_base::unregister_transaction(transaction_base *T)
	noexcept
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


bool pqxx::connection_base::read_copy_line(std::string &Line)
{
  if (!is_open())
    throw internal_error("read_copy_line() without connection");

  Line.erase();
  bool Result;

  char *Buf = nullptr;
  const std::string query = "[END COPY]";
  const auto line_len = PQgetCopyData(m_conn, &Buf, false);
  switch (line_len)
  {
  case -2:
    throw failure("Reading of table data failed: " + std::string(err_msg()));

  case -1:
    for (
	auto R = make_result(PQgetResult(m_conn), query);
        gate::result_connection(R);
	R=make_result(PQgetResult(m_conn), query)
	)
      check_result(R);
    Result = false;
    break;

  case 0:
    throw internal_error("table read inexplicably went asynchronous");

  default:
    if (Buf)
    {
      std::unique_ptr<char, void (*)(char *)> PQA(
          Buf, freepqmem_templated<char>);
      Line.assign(Buf, line_len);
    }
    Result = true;
  }

  return Result;
}


void pqxx::connection_base::write_copy_line(const std::string &Line)
{
  if (!is_open())
    throw internal_error("write_copy_line() without connection");

  const std::string L = Line + '\n';
  const char *const LC = L.c_str();
  const auto Len = L.size();

  if (PQputCopyData(m_conn, LC, int(Len)) <= 0)
  {
    const std::string msg = (
        std::string("Error writing to table: ") + err_msg());
    PQendcopy(m_conn);
    throw failure(msg);
  }
}


void pqxx::connection_base::end_copy_write()
{
  int Res = PQputCopyEnd(m_conn, nullptr);
  switch (Res)
  {
  case -1:
    throw failure("Write to table failed: " + std::string(err_msg()));
  case 0:
    throw internal_error("table write is inexplicably asynchronous");
  case 1:
    // Normal termination.  Retrieve result object.
    break;

  default:
    throw internal_error(
	"unexpected result " + to_string(Res) + " from PQputCopyEnd()");
  }

  check_result(make_result(PQgetResult(m_conn), "[END COPY]"));
}


void pqxx::connection_base::start_exec(const std::string &Q)
{
  activate();
  if (!PQsendQuery(m_conn, Q.c_str())) throw failure(err_msg());
}


pqxx::internal::pq::PGresult *pqxx::connection_base::get_result()
{
  if (!m_conn) throw broken_connection();
  return PQgetResult(m_conn);
}


void pqxx::connection_base::add_reactivation_avoidance_count(int n)
{
  m_reactivation_avoidance.add(n);
}


std::string pqxx::connection_base::esc(const char str[], size_t maxlen)
{
  // We need a connection object...  This is the one reason why this function is
  // not const!
  if (!m_conn) activate();

  std::vector<char> buf(2 * maxlen + 1);
  int err = 0;
  PQescapeStringConn(m_conn, &buf[0], str, maxlen, &err);
  if (err) throw argument_error(err_msg());
  return std::string(&buf[0]);
}


std::string pqxx::connection_base::esc(const char str[])
{
  return this->esc(str, strlen(str));
}


std::string pqxx::connection_base::esc(const std::string &str)
{
  return this->esc(str.c_str(), str.size());
}


std::string pqxx::connection_base::esc_raw(
        const unsigned char str[],
        size_t len)
{
  size_t bytes = 0;
  // We need a connection object...  This is the one reason why this function is
  // not const!
  activate();

  std::unique_ptr<unsigned char, void (*)(unsigned char *)> buf(
	PQescapeByteaConn(m_conn, str, len, &bytes),
	freepqmem_templated<unsigned char>);
  if (!buf.get()) throw std::bad_alloc();
  return std::string(reinterpret_cast<char *>(buf.get()));
}


std::string pqxx::connection_base::unesc_raw(const char *text)
{
  size_t len;
  unsigned char *bytes = const_cast<unsigned char *>(
	reinterpret_cast<const unsigned char *>(text));
  const unsigned char *const buf = PQunescapeBytea(bytes, &len);
  return std::string(buf, buf + len);
}


std::string pqxx::connection_base::quote_raw(
        const unsigned char str[],
        size_t len)
{
  return "'" + esc_raw(str, len) + "'::bytea";
}


std::string pqxx::connection_base::quote(const binarystring &b)
{
  return quote_raw(b.data(), b.size());
}


std::string pqxx::connection_base::quote_name(const std::string &identifier)
{
  // We need a connection object...  This is the one reason why this function is
  // not const!
  activate();
  std::unique_ptr<char, void (*)(char *)> buf(
	PQescapeIdentifier(m_conn, identifier.c_str(), identifier.size()),
        freepqmem_templated<char>);
  if (!buf.get()) throw failure(err_msg());
  return std::string(buf.get());
}


pqxx::internal::reactivation_avoidance_exemption::
  reactivation_avoidance_exemption(
	connection_base &C) :
  m_home(C),
  m_count(gate::connection_reactivation_avoidance_exemption(C).get_counter()),
  m_open(C.is_open())
{
  gate::connection_reactivation_avoidance_exemption gate(C);
  gate.clear_counter();
}


pqxx::internal::reactivation_avoidance_exemption::
  ~reactivation_avoidance_exemption()
{
  // Don't leave the connection open if reactivation avoidance is in effect and
  // the connection needed to be reactivated temporarily.
  if (m_count && !m_open) m_home.deactivate();
  gate::connection_reactivation_avoidance_exemption gate(m_home);
  gate.add_counter(m_count);
}


namespace
{
#if defined(_WIN32) || defined(HAVE_POLL)
// Convert a timeval to milliseconds, or -1 if no timeval is given.
inline int tv_milliseconds(timeval *tv = nullptr)
{
  return tv ? int(tv->tv_sec * 1000 + tv->tv_usec/1000) : -1;
}
#endif


/// Wait for an fd to become free for reading/writing.  Optional timeout.
void wait_fd(int fd, bool forwrite=false, timeval *tv=nullptr)
{
  if (fd < 0) throw pqxx::broken_connection();

// WSAPoll is available in winsock2.h only for versions of Windows >= 0x0600
#if defined(_WIN32) && (_WIN32_WINNT >= 0x0600)
  const short events = (forwrite ? POLLWRNORM : POLLRDNORM);
  WSAPOLLFD fdarray{SOCKET(fd), events, 0};
  WSAPoll(&fdarray, 1, tv_milliseconds(tv));
#elif defined(HAVE_POLL)
  const short events = short(
        POLLERR|POLLHUP|POLLNVAL | (forwrite?POLLOUT:POLLIN));
  pollfd pfd{fd, events, 0};
  poll(&pfd, 1, tv_milliseconds(tv));
#else
  // No poll()?  Our last option is select().
  fd_set read_fds;
  FD_ZERO(&read_fds);
  if (!forwrite) FD_SET(fd, &read_fds);

  fd_set write_fds;
  FD_ZERO(&write_fds);
  if (forwrite) FD_SET(fd, &write_fds);

  fd_set except_fds;
  FD_ZERO(&except_fds);
  FD_SET(fd, &except_fds);

  select(fd+1, &read_fds, &write_fds, &except_fds, tv);
#endif

  // No need to report errors.  The caller will try to use the file
  // descriptor right after we return, so if the file descriptor is broken,
  // the caller will notice soon enough.
}
} // namespace

void pqxx::internal::wait_read(const internal::pq::PGconn *c)
{
  wait_fd(socket_of(c));
}


void pqxx::internal::wait_read(
	const internal::pq::PGconn *c,
	long seconds,
	long microseconds)
{
  // These are really supposed to be time_t and suseconds_t.  But not all
  // platforms have that type; some use "long" instead, and some 64-bit
  // systems use 32-bit integers here.  So "int" seems to be the only really
  // safe type to use.
  timeval tv = { time_t(seconds), int(microseconds) };
  wait_fd(socket_of(c), false, &tv);
}


void pqxx::internal::wait_write(const internal::pq::PGconn *c)
{
  wait_fd(socket_of(c), true);
}


void pqxx::connection_base::wait_read() const
{
  internal::wait_read(m_conn);
}


void pqxx::connection_base::wait_read(long seconds, long microseconds) const
{
  internal::wait_read(m_conn, seconds, microseconds);
}


void pqxx::connection_base::wait_write() const
{
  internal::wait_write(m_conn);
}


int pqxx::connection_base::await_notification()
{
  activate();
  int notifs = get_notifs();
  if (!notifs)
  {
    wait_read();
    notifs = get_notifs();
  }
  return notifs;
}


int pqxx::connection_base::await_notification(long seconds, long microseconds)
{
  activate();
  int notifs = get_notifs();
  if (!notifs)
  {
    wait_read(seconds, microseconds);
    notifs = get_notifs();
  }
  return notifs;
}


void pqxx::connection_base::read_capabilities()
{
  m_serverversion = PQserverVersion(m_conn);
  if (m_serverversion <= 90000)
    throw feature_not_supported(
	"Unsupported server version; 9.0 is the minimum.");

  switch (protocol_version()) {
  case 0:
    throw broken_connection();
  case 1:
  case 2:
    throw feature_not_supported(
        "Unsupported frontend/backend protocol version; 3.0 is the minimum.");
  default:
    break;
  }

  // TODO: Check for capabilities here.  Currently don't need any checks.
}


std::string pqxx::connection_base::adorn_name(const std::string &n)
{
  const std::string id = to_string(++m_unique_id);
  return n.empty() ? ("x"+id) : (n+"_"+id);
}


int pqxx::connection_base::encoding_code()
{
  activate();
  return PQclientEncoding(m_conn);
}


pqxx::result pqxx::connection_base::parameterized_exec(
	const std::string &query,
	const char *const params[],
	const int paramlengths[],
	const int binaries[],
	int nparams)
{
  auto r = make_result(
  	PQexecParams(
		m_conn,
		query.c_str(),
		nparams,
		nullptr,
		params,
		paramlengths,
		binaries,
		0),
	query);
  check_result(r);
  get_notifs();
  return r;
}


pqxx::result pqxx::connection_base::exec_params(
	const std::string &query,
	const internal::params &args)
{
  const auto pointers = args.get_pointers();
  const auto pq_result = PQexecParams(
	m_conn,
	query.c_str(),
	int(args.nonnulls.size()),
	nullptr,
	&pointers[0],
	&args.lengths[0],
	&args.binaries[0],
	0);
  const auto r = make_result(pq_result, query);
  check_result(r);
  get_notifs();
  return r;
}
