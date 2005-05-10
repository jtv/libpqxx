/*-------------------------------------------------------------------------
 *
 *   FILE
 *	connection_base.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::connection_base abstract base class.
 *   pqxx::connection_base encapsulates a frontend to backend connection
 *
 * Copyright (c) 2001-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <stdexcept>

#ifdef PQXX_HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif	// HAVE_UNISTD_H
#endif	// PQXX_HAVE_SYS_SELECT_H

#include "libpq-fe.h"

#include "pqxx/connection_base"
#include "pqxx/nontransaction"
#include "pqxx/pipeline"
#include "pqxx/result"
#include "pqxx/transaction"
#include "pqxx/trigger"

using namespace PGSTD;
using namespace pqxx::internal;


extern "C"
{
// Pass C-linkage notice processor call on to C++-linkage noticer object.  The
// void * argument points to the noticer.
static void pqxxNoticeCaller(void *arg, const char *Msg)
{
  if (arg && Msg) (*static_cast<pqxx::noticer *>(arg))(Msg);
}
}


pqxx::connection_base::connection_base(const string &ConnInfo) :
  m_ConnInfo(ConnInfo),
  m_Conn(0),
  m_Trans(),
  m_Noticer(),
  m_Trace(0),
  m_fdmask()
{
  clear_fdmask();
}


pqxx::connection_base::connection_base(const char ConnInfo[]) :
  m_ConnInfo(ConnInfo ? ConnInfo : ""),
  m_Conn(0),
  m_Trans(),
  m_Noticer(),
  m_Trace(0),
  m_fdmask()
{
  clear_fdmask();
}


int pqxx::connection_base::backendpid() const throw ()
{
  return m_Conn ? PQbackendPID(m_Conn) : 0;
}


void pqxx::connection_base::activate()
{
  if (!is_open())
  {
    startconnect();
    completeconnect();

    if (!is_open())
    {
      const string Msg( ErrMsg() );
      disconnect();
      throw broken_connection(Msg);
    }

    SetupState();
  }
}


void pqxx::connection_base::deactivate()
{
  if (m_Conn)
  {
    if (m_Trans.get())
      throw logic_error("Attempt to deactivate connection while " +
	  		m_Trans.get()->description() + " still open");
  }
  dropconnect();
  disconnect();
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
    throw logic_error("libpqxx internal error: SetupState() on no connection");

  if (Status() != CONNECTION_OK)
  {
    const string Msg( ErrMsg() );
    dropconnect();
    disconnect();
    throw runtime_error(Msg);
  }

  for (PSMap::iterator p = m_prepared.begin(); p != m_prepared.end(); ++p)
    p->second.registered = false;

  if (m_Noticer.get())
    PQsetNoticeProcessor(m_Conn, pqxxNoticeCaller, m_Noticer.get());

  InternalSetTrace();

  if (!m_Triggers.empty() || !m_Vars.empty())
  {
    // Pipeline all queries needed to restore triggers and variables.  Use
    // existing nontransaction if we're in one, or set up a temporary one.
    // Note that at this stage, the presence of a transaction object doesn't
    // mean much.  Even if it is a dbtransaction, no BEGIN has been issued yet
    // in the new connection.
    auto_ptr<nontransaction> n;
    if (!m_Trans.get())
    {
      // Jump through a few hoops to satisfy dear old g++ 2.95
      auto_ptr<nontransaction>
	nap(new nontransaction(*this, "connection_setup"));
      n = nap;
    }
    pipeline p(*m_Trans.get(), "restore_state");
    p.retain(m_Triggers.size() + m_Vars.size());

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
	  p.insert("LISTEN \"" + i->first + "\"");
          Last = i->first;
        }
      }
    }

    for (map<string,string>::const_iterator i=m_Vars.begin();
         i!=m_Vars.end();
         ++i)
      p.insert("SET " + i->first + "=" + i->second);

    // Now do the whole batch at once
    while (!p.empty()) p.retrieve();
  }
}


void pqxx::connection_base::disconnect() throw ()
{
  if (m_Conn)
  {
    PQfinish(m_Conn);
    m_Conn = 0;
  }
}


bool pqxx::connection_base::is_open() const throw ()
{
  return m_Conn && (Status() == CONNECTION_OK);
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

    try
    {
      R.CheckStatus(LQ);
    }
    catch (const broken_connection &)
    {
    }
    catch (const exception &)
    {
      if (is_open()) throw;
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


void pqxx::connection_base::consume_input() throw ()
{
  PQconsumeInput(m_Conn);
}


bool pqxx::connection_base::is_busy() const throw ()
{
  return PQisBusy(m_Conn);
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

  if (!R)
  {
    // A shame we can't detect out-of-memory to turn this into a bad_alloc...
    if (!is_open()) throw broken_connection();
    throw runtime_error(ErrMsg());
  }
  R.CheckStatus(Query);
  get_notifs();
  return R;
}


void pqxx::connection_base::pq_prepare(const string &name, 
    const string &def,
    const string &params)
{
  PSMap::iterator i = m_prepared.find(name);
  if (i != m_prepared.end())
  {
    // Quietly accept identical redefinitions
    if (def == i->second.definition && params == i->second.parameters) return;
    // Reject non-identical redefinitions
    throw logic_error("Incompatible redefinition of prepared statement "+name);
  }

  m_prepared.insert(PSMap::value_type(name, prepared_def(def, params)));
}


void pqxx::connection_base::unprepare(const string &name)
{
  PSMap::iterator i = m_prepared.find(name);

  // Quietly ignore duplicated or spurious unprepare()s
  if (i == m_prepared.end()) return;

  if (i->second.registered) Exec(("DEALLOCATE " + name).c_str(), 0);

  m_prepared.erase(i);
}


pqxx::result pqxx::connection_base::pq_exec_prepared(const string &pname,
	int nparams,
	const char *const *params)
{
  activate();

  PSMap::iterator p = m_prepared.find(pname);
  if (p == m_prepared.end())
    throw logic_error("Unknown prepared statement: " + pname);

  // "Register" (i.e., define) prepared statement with backend on demand
  if (!p->second.registered)
  {
    // TODO: Wrap in nested transaction if backend supports it!
    stringstream P;
    P << "PREPARE " << pname << ' ' << p->second.parameters 
      << " AS " << p->second.definition;
    Exec(P.str().c_str(), 0);
    p->second.registered = true;
  }

#ifdef PQXX_HAVE_PQEXECPREPARED
  result R(PQexecPrepared(m_Conn, pname.c_str(), nparams, params, 0, 0, 0));

  if (!R)
  {
    if (!is_open()) throw broken_connection();
    throw runtime_error(ErrMsg());
  }
  R.CheckStatus(pname);
  get_notifs();
  return R;
#else
  stringstream Q;
  Q << "EXECUTE "
    << pname
    << ' '
    << separated_list(",", params, params+nparams);
  return Exec(Q.str().c_str(), 0);
#endif
}


void pqxx::connection_base::Reset()
{
  clear_fdmask();

  // Forget about any previously ongoing connection attempts
  dropconnect();

  if (m_Conn)
  {
    // Reset existing connection
    PQreset(m_Conn);
    SetupState();
    clear_fdmask();
  }
  else
  {
    // No existing connection--start a new one
    activate();
  }
}


void pqxx::connection_base::close() throw ()
{
#ifdef PQXX_QUIET_DESTRUCTORS
  set_noticer(PGSTD::auto_ptr<noticer>(new nonnoticer()));
#endif
  clear_fdmask();
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

    disconnect();
  }
  catch (...)
  {
  }
  clear_fdmask();
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
    throw logic_error("libpqxx internal error: MakeEmpty() on null connection");

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
    throw logic_error("libpqxx internal error: "
	              "ReadCopyLine() without connection");

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
	R.CheckStatus("[END COPY]");
      Result = false;
      break;

    case 0:
      throw logic_error("libpqxx internal error: "
	  "table read inexplicably went asynchronous");

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
    throw logic_error("libpqxx internal error: "
	              "WriteCopyLine() without connection");

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
    throw logic_error("libpqxx internal error: "
	    "table write is inexplicably asynchronous");
  case 1:
    // Normal termination.  Retrieve result object.
    break;

  default:
    throw logic_error("libpqxx internal error: "
	    "unexpected result " + to_string(Res) + " from PQputCopyEnd()");
  }

  const result R(PQgetResult(m_Conn));
  R.CheckStatus("[END COPY]");

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
#ifdef PQXX_SELECT_ACCEPTS_NULL
  // The always-empty fd_set
  fd_set *const fdset_none = 0;
#else	// PQXX_SELECT_ACCEPTS_NULL
  fd_set emptyfd;	// Relies on zero-initialization
  fd_set *const fdset_none = &emptyfd;
#endif	// PQXX_SELECT_ACCEPTS_NULL
} // namespace


int pqxx::connection_base::set_fdmask() const
{
  if (!m_Conn) throw broken_connection();
  const int fd = PQsocket(m_Conn);
  if (fd < 0) throw broken_connection();
  FD_SET(fd, &m_fdmask);
  return fd;
}


void pqxx::connection_base::clear_fdmask() throw ()
{
  FD_ZERO(&m_fdmask);
}

void pqxx::connection_base::wait_read() const
{
  const int fd = set_fdmask();
  select(fd+1, &m_fdmask, fdset_none, &m_fdmask, 0);
}


void pqxx::connection_base::wait_read(long seconds, long microseconds) const
{
  timeval tv = { seconds, microseconds };
  const int fd = set_fdmask();
  select(fd+1, &m_fdmask, fdset_none, &m_fdmask, &tv);
}


void pqxx::connection_base::wait_write() const
{
  const int fd = set_fdmask();
  select(fd+1, fdset_none, &m_fdmask, &m_fdmask, 0);
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


