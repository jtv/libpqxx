/*-------------------------------------------------------------------------
 *
 *   FILE
 *	connection_base.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::connection_base abstract base class.
 *   pqxx::connection_base encapsulates a frontend to backend connection
 *
 * Copyright (c) 2001-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdio>
#include <ctime>
#include <stdexcept>

#ifdef PQXX_HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#endif
#endif	// PQXX_HAVE_SYS_SELECT_H

#include "libpq-fe.h"

#include "pqxx/binarystring"
#include "pqxx/connection"
#include "pqxx/connection_base"
#include "pqxx/nontransaction"
#include "pqxx/pipeline"
#include "pqxx/result"
#include "pqxx/transaction"
#include "pqxx/trigger"

using namespace PGSTD;
using namespace pqxx;
using namespace pqxx::internal;
using namespace pqxx::prepare;


extern "C"
{
// Pass C-linkage notice processor call on to C++-linkage noticer object.  The
// void * argument points to the noticer.
static void pqxxNoticeCaller(void *arg, const char *Msg)
{
  if (arg && Msg) (*static_cast<pqxx::noticer *>(arg))(Msg);
}


#ifdef PQXX_SELECT_ACCEPTS_NULL
  // The always-empty fd_set
static fd_set *const fdset_none = 0;
#else	// PQXX_SELECT_ACCEPTS_NULL
static fd_set emptyfd;	// Relies on zero-initialization
static fd_set *const fdset_none = &emptyfd;
#endif	// PQXX_SELECT_ACCEPTS_NULL



// Concentrate stupid "old-style cast" warnings for GNU libc in one place, and
// by using "C" linkage, perhaps silence them altogether.
static void set_fdbit(int f, fd_set *s)
{
  FD_SET(f, s);
}

static void clear_fdmask(fd_set *mask)
{
  FD_ZERO(mask);
}
}


pqxx::connection_base::connection_base(connectionpolicy &pol) :
  m_Conn(0),
  m_policy(pol),
  m_Completed(false),
  m_Trans(),
  m_Noticer(),
  m_Trace(0),
  m_caps(),
  m_inhibit_reactivation(false),
  m_reactivation_avoidance(),
  m_unique_id(0)
{
}


void pqxx::connection_base::init()
{
  m_Conn = m_policy.do_startconnect(m_Conn);
}


int pqxx::connection_base::backendpid() const throw ()
{
  return m_Conn ? PQbackendPID(m_Conn) : 0;
}


namespace
{
int socket_of(const ::pqxx::internal::pq::PGconn *c)
{
  return c ? PQsocket(c) : -1;
}
}


int pqxx::connection_base::sock() const throw ()
{
  return socket_of(m_Conn);
}


void pqxx::connection_base::activate()
{
  if (!is_open()) try
  {
    if (m_inhibit_reactivation)
      throw broken_connection("Could not reactivate connection; "
	  "reactivation is inhibited");

    // If any objects were open that didn't survive the closing of our
    // connection, don't try to reactivate
    if (m_reactivation_avoidance.get()) return;

    m_Conn = m_policy.do_startconnect(m_Conn);
    m_Conn = m_policy.do_completeconnect(m_Conn);
    m_Completed = true;	// (But retracted if error is thrown below)

    if (!is_open()) throw broken_connection();

    SetupState();
  }
  catch (const broken_connection &e)
  {
    const string Msg( ErrMsg() );
    disconnect();
    m_Completed = false;
    throw broken_connection(e.what());
  }
  catch (const exception &)
  {
    m_Completed = false;
    throw;
  }
}


void pqxx::connection_base::deactivate()
{
  if (!m_Conn) return;

  if (m_Trans.get())
    throw logic_error("Attempt to deactivate connection while " +
	m_Trans.get()->description() + " still open");

  if (m_reactivation_avoidance.get())
  {
    process_notice("Attempt to deactivate connection while it is in a state "
	"that cannot be fully recovered later (ignoring)");
    return;
  }

  m_Completed = false;
  m_Conn = m_policy.do_disconnect(m_Conn);
}


void pqxx::connection_base::set_variable(const string &Var, const string &Value)
{
  if (m_Trans.get())
  {
    // We're in a transaction.  The variable should go in there.
    m_Trans.get()->set_variable(Var, Value);
  }
  else
  {
    // We're not in a transaction.  Set a session variable.
    if (is_open()) RawSetVar(Var, Value);
    m_Vars[Var] = Value;
  }
}


string pqxx::connection_base::get_variable(const string &Var)
{
  return m_Trans.get() ? m_Trans.get()->get_variable(Var) : RawGetVar(Var);
}


string pqxx::connection_base::RawGetVar(const string &Var)
{
  // Is this variable in our local map of set variables?
  // TODO: Is it safe to read-allocate variables in the "cache?"
  map<string,string>::const_iterator i=m_Vars.find(Var);
  if (i != m_Vars.end()) return i->second;

  return Exec(("SHOW " + Var).c_str(), 0).at(0).at(0).as(string());
}


/** Set up various parts of logical connection state that may need to be
 * recovered because the physical connection to the database was lost and is
 * being reset, or that may not have been initialized yet.
 */
void pqxx::connection_base::SetupState()
{
  if (!m_Conn)
    throw internal_error("SetupState() on no connection");

  if (Status() != CONNECTION_OK)
  {
    const string Msg( ErrMsg() );
    m_Conn = m_policy.do_disconnect(m_Conn);
    throw runtime_error(Msg);
  }

  read_capabilities();

  const PSMap::const_iterator prepared_end(m_prepared.end());
  for (PSMap::iterator p = m_prepared.begin(); p != prepared_end; ++p)
    p->second.registered = false;

  if (m_Noticer.get())
    PQsetNoticeProcessor(m_Conn, pqxxNoticeCaller, m_Noticer.get());

  InternalSetTrace();

  if (!m_Triggers.empty() || !m_Vars.empty())
  {
    stringstream restore_query;

    // Pipeline all queries needed to restore triggers and variables, so we can
    // send them over in one go.

    // Reinstate all active triggers
    if (!m_Triggers.empty())
    {
      const TriggerList::const_iterator End = m_Triggers.end();
      string Last;
      for (TriggerList::const_iterator i = m_Triggers.begin(); i != End; ++i)
      {
        // m_Triggers can handle multiple Triggers waiting on the same event;
        // issue just one LISTEN for each event.
        if (i->first != Last)
        {
          restore_query << "LISTEN \"" << i->first << "\"; ";
          Last = i->first;
        }
      }
    }

    const map<string,string>::const_iterator var_end(m_Vars.end());
    for (map<string,string>::const_iterator i=m_Vars.begin(); i!=var_end; ++i)
      restore_query << "SET " << i->first << "=" << i->second << "; ";

    // Now do the whole batch at once
    PQsendQuery(m_Conn, restore_query.str().c_str());
    result r;
    do r = PQgetResult(m_Conn); while (r);
  }

  m_Completed = true;
  if (!is_open()) throw broken_connection();
}


void pqxx::connection_base::check_result(const result &R, const char Query[])
{
  if (!is_open()) throw broken_connection();

  // A shame we can't detect out-of-memory to turn this into a bad_alloc...
  if (!R) throw runtime_error(ErrMsg());

  try
  {
    R.CheckStatus(Query);
  }
  catch (const exception &e)
  {
    /* If the connection is broken, we'd expect is_open() to return false, since
     * PQstatus() is supposed to return CONNECTION_BAD.
     *
     * It turns out that, at least for the libpq in PostgreSQL 8.0 and the 8.1
     * prerelease I'm looking at now (we're writing July 2005), libpq will only
     * abandon CONNECTION_OK if the errno code for the broken connection is
     * exactly EPIPE or ECONNRESET.  This is usually fine for local connections,
     * but ignores the wide range of fatal errors that can occur on TCP/IP
     * sockets, such as extreme out-of-memory conditions, hardware failures,
     * severed network connections, and serious software errors.  In these cases
     * libpq will return an error result that is (as far as I can make out)
     * indistinguishable from a server-side error, but will retain its state
     * indication of CONNECTION_OK--thus inviting the possibly false impression
     * that the query failed to execute (in reality it may have executed
     * successfully) and the definitely false impression that the connection is
     * still in a workable state.
     *
     * A particular worry is connection timeout.  One user observed a 15-minute
     * period of inactivity when he pulled out a network cable, after which
     * libpq finally returned a result object containing an error (the error
     * message for connection failure is subject to translation, by the way, and
     * is also subject to a bug where random data may currently be embedded in
     * it in place of the system's explanation of the error, so even attempting
     * to recognize the string is not a reliable workaround) but said the
     * connection was still operational.
     *
     * This bug seems to have been fixed in the libpq that shipped with
     * PostgreSQL 8.1.
     */
    // TODO: Only do this extra check when using buggy libpq!
    if (!consume_input()) throw broken_connection(e.what());
    const int fd = sock();
    if (fd < 0) throw broken_connection(e.what());
    fd_set errs;
    clear_fdmask(&errs);
    int sel;
    do
    {
      timeval nowait = { 0, 0 };
      set_fdbit(fd, &errs);
      sel = select(fd+1, 0, 0, &errs, &nowait);
    } while (sel == -1 && errno == EINTR);

    switch (sel)
    {
    case -1:
      switch (errno)
      {
      case EBADF:
      case EINVAL:
        throw broken_connection(e.what());
      case ENOMEM:
        throw bad_alloc();
      }
    case 0:
      break;
    case 1:
      throw broken_connection(e.what());
    }

    throw;
  }
}


void pqxx::connection_base::disconnect() throw ()
{
  // When we activate again, the server may be different!
  for (int i=0; i<cap_end; ++i) m_caps[i] = false;

  m_Conn = m_policy.do_disconnect(m_Conn);
}


bool pqxx::connection_base::is_open() const throw ()
{
  return m_Conn && m_Completed && (Status() == CONNECTION_OK);
}


auto_ptr<pqxx::noticer>
pqxx::connection_base::set_noticer(auto_ptr<noticer> N) throw ()
{
  if (m_Conn)
  {
    if (N.get()) PQsetNoticeProcessor(m_Conn, pqxxNoticeCaller, N.get());
    else PQsetNoticeProcessor(m_Conn, 0, 0);
  }

  auto_ptr<noticer> Old = m_Noticer;
  m_Noticer = N;

  return Old;
}


void pqxx::connection_base::process_notice_raw(const char msg[]) throw ()
{
  if (msg && *msg)
  {
    // TODO: Read default noticer on startup!
    if (m_Noticer.get()) (*m_Noticer.get())(msg);
    else fputs(msg, stderr);
  }
}


void pqxx::connection_base::process_notice(const char msg[]) throw ()
{
  if (!msg)
  {
    process_notice_raw("NULL pointer in client program message!\n");
  }
  else
  {
    const size_t len = strlen(msg);
    if (len > 0)
    {
      if (msg[len-1] == '\n')
      {
        process_notice_raw(msg);
      }
      else try
      {
        // Newline is missing.  Try the C++ string version of this function.
        process_notice(string(msg));
      }
      catch (const exception &)
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
  }
}

void pqxx::connection_base::process_notice(const string &msg) throw ()
{
  // Ensure that message passed to noticer ends in newline
  if (msg[msg.size()-1] == '\n')
  {
    process_notice_raw(msg.c_str());
  }
  else try
  {
    const string nl = msg + "\n";
    process_notice_raw(nl.c_str());
  }
  catch (const exception &)
  {
    // If nothing else works, try writing the message without the newline
    process_notice_raw(msg.c_str());
    // This is ugly.
    process_notice_raw("\n");
  }
}


void pqxx::connection_base::trace(FILE *Out) throw ()
{
  m_Trace = Out;
  if (m_Conn) InternalSetTrace();
}


void pqxx::connection_base::AddTrigger(pqxx::trigger *T)
{
  if (!T) throw invalid_argument("Null trigger registered");

  // Add to triggers list and attempt to start listening.
  const TriggerList::iterator p = m_Triggers.find(T->name());
  const TriggerList::value_type NewVal(T->name(), T);

  if (m_Conn && (p == m_Triggers.end()))
  {
    // Not listening on this event yet, start doing so.
    const string LQ("LISTEN \"" + T->name() + "\"");
    result R( PQexec(m_Conn, LQ.c_str()) );

    if (is_open()) try
    {
      check_result(R,LQ.c_str());
    }
    catch (const broken_connection &)
    {
    }
    m_Triggers.insert(NewVal);
  }
  else
  {
    m_Triggers.insert(p, NewVal);
  }

}


void pqxx::connection_base::RemoveTrigger(pqxx::trigger *T) throw ()
{
  if (!T) return;

  try
  {
    // Keep Sun compiler happy...  Hope it doesn't annoy other compilers
    pair<const string, trigger *> tmp_pair(T->name(), T);
    TriggerList::value_type E = tmp_pair;

    typedef pair<TriggerList::iterator, TriggerList::iterator> Range;
    Range R = m_Triggers.equal_range(E.first);

    const TriggerList::iterator i = find(R.first, R.second, E);

    if (i == R.second)
    {
      process_notice("Attempt to remove unknown trigger '" +
		     E.first +
		     "'");
    }
    else
    {
      // Erase first; otherwise a notification for the same trigger may yet come
      // in and wreak havoc.  Thanks Dragan Milenkovic. 
      m_Triggers.erase(i);

      if (m_Conn && (R.second == ++R.first))
        Exec(("UNLISTEN \"" + T->name() + "\"").c_str(), 0);
    }
  }
  catch (const exception &e)
  {
    process_notice(e.what());
  }
}


bool pqxx::connection_base::consume_input() throw ()
{
  return PQconsumeInput(m_Conn) != 0;
}


bool pqxx::connection_base::is_busy() const throw ()
{
  return PQisBusy(m_Conn) != 0;
}


int pqxx::connection_base::get_notifs()
{
  int notifs = 0;
  if (!is_open()) return notifs;

  PQconsumeInput(m_Conn);

  // Even if somehow we receive notifications during our transaction, don't
  // deliver them.
  if (m_Trans.get()) return notifs;

  for (PQAlloc<PGnotify> N( PQnotifies(m_Conn) ); N; N = PQnotifies(m_Conn))
  {
    typedef TriggerList::iterator TI;

    notifs++;

    pair<TI, TI> Hit = m_Triggers.equal_range(string(N->relname));
    for (TI i = Hit.first; i != Hit.second; ++i) try
    {
      (*i->second)(N->be_pid);
    }
    catch (const exception &e)
    {
      try
      {
        process_notice("Exception in trigger handler '" +
		       i->first +
		       "': " +
		       e.what() +
		       "\n");
      }
      catch (const bad_alloc &)
      {
        // Out of memory.  Try to get the message out in a more robust way.
        process_notice("Exception in trigger handler, "
	    "and also ran out of memory\n");
      }
      catch (const exception &)
      {
        process_notice("Exception in trigger handler "
	    "(compounded by other error)\n");
      }
    }

    N.clear();
  }
  return notifs;
}


const char *pqxx::connection_base::dbname()
{
  if (!m_Conn) activate();
  return PQdb(m_Conn);
}


const char *pqxx::connection_base::username()
{
  if (!m_Conn) activate();
  return PQuser(m_Conn);
}


const char *pqxx::connection_base::hostname()
{
  if (!m_Conn) activate();
  return PQhost(m_Conn);
}


const char *pqxx::connection_base::port()
{
  if (!m_Conn) activate();
  return PQport(m_Conn);
}


const char *pqxx::connection_base::ErrMsg() const throw ()
{
  return m_Conn ? PQerrorMessage(m_Conn) : "No connection to database";
}


pqxx::result pqxx::connection_base::Exec(const char Query[], int Retries)
{
  activate();

  result R( PQexec(m_Conn, Query) );

  while ((Retries > 0) && !R && !is_open())
  {
    Retries--;
    Reset();
    if (is_open()) R = PQexec(m_Conn, Query);
  }

  check_result(R, Query);

  get_notifs();
  return R;
}


pqxx::prepare::declaration pqxx::connection_base::prepare(const string &name,
    const string &definition)
{
  PSMap::iterator i = m_prepared.find(name);
  if (i != m_prepared.end())
  {
    if (definition != i->second.definition)
      throw invalid_argument("Inconsistent redefinition "
	  "of prepared statement " + name);

    // Prepare for repeated repetition of parameters
    i->second.parameters.clear();
    i->second.complete = false;
  }
  else
  {
    m_prepared.insert(make_pair(name,
	  prepare::internal::prepared_def(definition)));
  }
  return prepare::declaration(*this,name);
}


void pqxx::connection_base::unprepare(const string &name)
{
  PSMap::iterator i = m_prepared.find(name);

  // Quietly ignore duplicated or spurious unprepare()s
  if (i == m_prepared.end()) return;

  if (i->second.registered) Exec(("DEALLOCATE " + name).c_str(), 0);

  m_prepared.erase(i);
}


namespace
{
string escape_param(const char in[], prepare::param_treatment treatment)
{
  if (!in) return "null";
  
  switch (treatment)
  {
  case treat_binary:
    return "'" + escape_binary(in) + "'";

  case treat_string:
    return "'" + sqlesc(in) + "'";

  case treat_bool:
    switch (in[0])
    {
    case 't':
    case 'T':
    case 'f':
    case 'F':
      break;
    default:
      {
        // Looks like a numeric value may have been passed.  Try to convert it
        // back to a number, then to bool to normalize its representation
        bool b;
	from_string(in,b);
        return to_string(b);
      }
    }
    break;

  case treat_direct:
    break;

  default:
    throw logic_error("Unknown treatment for prepared-statement parameter");
  }

  return in;
}
} // namespace


pqxx::prepare::internal::prepared_def &
pqxx::connection_base::find_prepared(const string &statement)
{
  PSMap::iterator s = m_prepared.find(statement);
  if (s == m_prepared.end())
    throw invalid_argument("Unknown prepared statement '" + statement + "'");
  return s->second;
}

void pqxx::connection_base::prepare_param_declare(const string &statement,
    const string &sqltype,
    param_treatment treatment)
{
  prepare::internal::prepared_def &s = find_prepared(statement);
  if (s.complete)
    throw logic_error("Attempt to add parameter to prepared statement " +
	statement +
	" after its definition was completed");
  s.addparam(sqltype,treatment);
}


pqxx::result pqxx::connection_base::prepared_exec(const string &statement,
	const char *const params[],
	int nparams)
{
  activate();

  prepare::internal::prepared_def &s = find_prepared(statement);

  if (nparams != int(s.parameters.size()))
    throw logic_error("Wrong number of parameters for prepared statement " +
	statement + ": "
	"expected " + to_string(s.parameters.size()) + ", "
	"received " + to_string(nparams));

  s.complete = true;

  // "Register" (i.e., define) prepared statement with backend on demand
  if (!s.registered && supports(cap_prepared_statements))
  {
#ifdef PQXX_HAVE_PQPREPARE
    PQprepare(m_Conn, statement.c_str(), s.definition.c_str(), 0, 0);
#else
    stringstream P;
    P << "PREPARE " << statement;

    if (!s.parameters.empty())
      P << '('
	<< separated_list(",",
	   	s.parameters.begin(),
		s.parameters.end(),
		prepare::internal::get_sqltype())
	<< ')';

    P << " AS " << s.definition;
    Exec(P.str().c_str(), 0);
#endif
    s.registered = true;
  }

#ifdef PQXX_HAVE_PQEXECPREPARED
  result r(PQexecPrepared(m_Conn,statement.c_str(),nparams,params,0,0,0));
#else
  stringstream Q;
  if (supports(cap_prepared_statements))
  {
    Q << "EXECUTE "
      << pname;
    if (nparams)
    {
      Q << " (";
      for (a = 0; a < nparams; ++a)
      {
	Q << escape_param(params[a],s.parameters[a].treatment);
	if (a < nparams-1) Q << ',';
      }
      Q << ')';
    }
  }
  else
  {
    // This backend doesn't support prepared statements.  Do our own variable
    // substitution.
    string S = s.definition;
    for (int n = nparams-1; n >= 0; --n)
    {
      const string key = "$" + to_string(n+1),
	           val = escape_param(params[n],s.parameters[a].treatment);
      const string::size_type keysz = key.size();
      for (string::size_type h=S.find(key); h!=string::npos; h=S.find(key))
	S.replace(h,keysz,val);
    }
    Q << S;
  }
  result r(Exec(Q.str().c_str(), 0));
#endif
  check_result(r, statement.c_str());
  get_notifs();
  return r;
}


void pqxx::connection_base::Reset()
{
  if (m_inhibit_reactivation)
    throw broken_connection("Could not reset connection: "
	"reactivation is inhibited");
  if (m_reactivation_avoidance.get()) return;

  // TODO: Probably need to go through a full disconnect/reconnect!
  // Forget about any previously ongoing connection attempts
  m_Conn = m_policy.do_dropconnect(m_Conn);
  m_Completed = false;

  if (m_Conn)
  {
    // Reset existing connection
    PQreset(m_Conn);
    SetupState();
  }
  else
  {
    // No existing connection--start a new one
    activate();
  }
}


void pqxx::connection_base::close() throw ()
{
  m_Completed = false;
#ifdef PQXX_QUIET_DESTRUCTORS
  auto_ptr<noticer> n(new nonnoticer());
  set_noticer(n);
#endif
  inhibit_reactivation(false);
  m_reactivation_avoidance.clear();
  try
  {
    if (m_Trans.get())
      process_notice("Closing connection while " +
	             m_Trans.get()->description() + " still open");

    if (!m_Triggers.empty())
    {
      process_notice("Closing connection with outstanding triggers");
      m_Triggers.clear();
    }

    m_Conn = m_policy.do_disconnect(m_Conn);
  }
  catch (...)
  {
  }
}


void pqxx::connection_base::RawSetVar(const string &Var, const string &Value)
{
    Exec(("SET " + Var + "=" + Value).c_str(), 0);
}


void pqxx::connection_base::AddVariables(const map<string,string> &Vars)
{
  for (map<string,string>::const_iterator i=Vars.begin(); i!=Vars.end(); ++i)
    m_Vars[i->first] = i->second;
}


void pqxx::connection_base::InternalSetTrace() throw ()
{
  if (m_Conn)
  {
    if (m_Trace) PQtrace(m_Conn, m_Trace);
    else PQuntrace(m_Conn);
  }
}


int pqxx::connection_base::Status() const throw ()
{
  return PQstatus(m_Conn);
}


void pqxx::connection_base::RegisterTransaction(transaction_base *T)
{
  m_Trans.Register(T);
}


void pqxx::connection_base::UnregisterTransaction(transaction_base *T)
  throw ()
{
  try
  {
    m_Trans.Unregister(T);
  }
  catch (const exception &e)
  {
    process_notice(e.what());
  }
}


void pqxx::connection_base::MakeEmpty(pqxx::result &R)
{
  if (!m_Conn)
    throw internal_error("MakeEmpty() on null connection");

  R = result(PQmakeEmptyPGresult(m_Conn, PGRES_EMPTY_QUERY));
}


#ifndef PQXX_HAVE_PQPUTCOPY
namespace
{
const string theWriteTerminator = "\\.";
}
#endif


bool pqxx::connection_base::ReadCopyLine(string &Line)
{
  if (!is_open())
    throw internal_error("ReadCopyLine() without connection");

  Line.erase();
  bool Result;

#ifdef PQXX_HAVE_PQPUTCOPY
  char *Buf = 0;
  switch (PQgetCopyData(m_Conn, &Buf, false))
  {
    case -2:
      throw runtime_error("Reading of table data failed: " + string(ErrMsg()));

    case -1:
      for (result R(PQgetResult(m_Conn)); R; R=PQgetResult(m_Conn))
	check_result(R, "[END COPY]");
      Result = false;
      break;

    case 0:
      throw internal_error("table read inexplicably went asynchronous");

    default:
      if (Buf)
      {
        PQAlloc<char> PQA(Buf);
        Line = Buf;
      }
      Result = true;
  }
#else
  char Buf[10000];
  bool LineComplete = false;

  while (!LineComplete)
  {
    switch (PQgetline(m_Conn, Buf, sizeof(Buf)))
    {
    case EOF:
      PQendcopy(m_Conn);
      throw runtime_error("Unexpected EOF from backend");

    case 0:
      LineComplete = true;
      break;

    case 1:
      break;

    default:
      throw runtime_error("Unexpected COPY response from backend");
    }

    Line += Buf;
  }

  Result = (Line != theWriteTerminator);

  if (Result)
  {
    if (!Line.empty() && (Line[Line.size()-1] == '\n'))
      Line.erase(Line.size()-1);
  }
  else
  {
    Line.erase();
    if (PQendcopy(m_Conn)) throw runtime_error(ErrMsg());
  }
#endif

  return Result;
}


void pqxx::connection_base::WriteCopyLine(const string &Line)
{
  if (!is_open())
    throw internal_error("WriteCopyLine() without connection");

  const string L = Line + '\n';
  const char *const LC = L.c_str();
  const string::size_type Len = L.size();

#ifdef PQXX_HAVE_PQPUTCOPY
  if (PQputCopyData(m_Conn, LC, Len) <= 0)
  {
    const string Msg = string("Error writing to table: ") + ErrMsg();
    PQendcopy(m_Conn);
    throw runtime_error(Msg);
  }
#else
  if (PQputnbytes(m_Conn, LC, Len) == EOF)
    throw runtime_error(string("Error writing to table: ") + ErrMsg());
#endif
}


void pqxx::connection_base::EndCopyWrite()
{
#ifdef PQXX_HAVE_PQPUTCOPY
  int Res = PQputCopyEnd(m_Conn, NULL);
  switch (Res)
  {
  case -1:
    throw runtime_error("Write to table failed: " + string(ErrMsg()));
  case 0:
    throw internal_error("table write is inexplicably asynchronous");
  case 1:
    // Normal termination.  Retrieve result object.
    break;

  default:
    throw internal_error("unexpected result " + to_string(Res) + " "
	"from PQputCopyEnd()");
  }

  const result R(PQgetResult(m_Conn));
  check_result(R, "[END COPY]");

#else
  WriteCopyLine(theWriteTerminator);
  // This check is a little odd, but for some reason PostgreSQL 7.4 keeps
  // returning 1 (i.e., failure) but with an empty error message, and without
  // anything seeming wrong.
  if ((PQendcopy(m_Conn) != 0) && ErrMsg() && *ErrMsg())
    throw runtime_error(ErrMsg());
#endif
}


void pqxx::connection_base::start_exec(const string &Q)
{
  activate();
  if (!PQsendQuery(m_Conn, Q.c_str())) throw runtime_error(ErrMsg());
}


pqxx::internal::pq::PGresult *pqxx::connection_base::get_result()
{
  if (!m_Conn) throw broken_connection();
  return PQgetResult(m_Conn);
}



namespace
{
void wait_fd(int fd, bool forwrite=false, timeval *tv=0)
{
  if (fd < 0) throw pqxx::broken_connection();
  fd_set s;
  clear_fdmask(&s);
  set_fdbit(fd, &s);
  select(fd+1, (forwrite?fdset_none:&s), (forwrite?&s:fdset_none), &s, tv);
}
} // namespace

void pqxx::internal::wait_read(const pq::PGconn *c)
{
  wait_fd(socket_of(c));
}


void pqxx::internal::wait_read(const pq::PGconn *c,
    long seconds,
    long microseconds)
{
  timeval tv = { seconds, microseconds };
  wait_fd(socket_of(c), false, &tv);
}


void pqxx::internal::wait_write(const pq::PGconn *c)
{
  wait_fd(socket_of(c), true);
}


void pqxx::connection_base::wait_read() const
{
  internal::wait_read(m_Conn);
}


void pqxx::connection_base::wait_read(long seconds, long microseconds) const
{
  internal::wait_read(m_Conn, seconds, microseconds);
}


void pqxx::connection_base::wait_write() const
{
  internal::wait_write(m_Conn);
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


void pqxx::connection_base::read_capabilities() throw ()
{
  int v = 0;
#ifdef PQXX_HAVE_PQSERVERVERSION
  if (m_Conn) v = PQserverVersion(m_Conn);
#endif

  m_caps[cap_prepared_statements] = (v >= 70300);
  m_caps[cap_cursor_scroll] = (v >= 70400);
  m_caps[cap_cursor_with_hold] = (v >= 70400);
  m_caps[cap_nested_transactions] = (v >= 80000);
  m_caps[cap_create_table_with_oids] = (v >= 80000);
}


string pqxx::connection_base::adorn_name(const string &n)
{
  const string id = to_string(++m_unique_id);
  return n.empty() ? ("x"+id) : (n+"_"+id);
}

