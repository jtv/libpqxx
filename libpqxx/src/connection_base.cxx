/*-------------------------------------------------------------------------
 *
 *   FILE
 *	connection_base.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::connection_base abstract base class.
 *   pqxx::connection_base encapsulates a frontend to backend connection
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
#include <unistd.h>
#endif	// PQXX_HAVE_SYS_SELECT_H

#include "pqxx/connection_base"
#include "pqxx/result"
#include "pqxx/transaction"
#include "pqxx/trigger"


using namespace PGSTD;


extern "C"
{
// Pass C-linkage notice processor call on to C++-linkage noticer object.  The
// void * argument points to the noticer.
void pqxxNoticeCaller(void *arg, const char *Msg)
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


void pqxx::connection_base::Connect()
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
      throw logic_error("Attempt to deactivate connection while transaction "
	                "'" + m_Trans.get()->name() + "' "
			"still open");
  }
  disconnect();
}


/// Initiate a connection, to at least the point where we have a PQconn
void pqxx::connection_base::halfconnect()
{
  startconnect();
  if (!m_Conn) completeconnect();
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
    disconnect();
    throw runtime_error(Msg);
  }

  if (m_Noticer.get())
    PQsetNoticeProcessor(m_Conn, pqxxNoticeCaller, m_Noticer.get());

  InternalSetTrace();

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
	const string LQ("LISTEN " + i->first);
        result R( PQexec(m_Conn, LQ.c_str()) );
        R.CheckStatus(LQ);
        Last = i->first;
      }
    }
  }

  for (map<string,string>::const_iterator i=m_Vars.begin(); 
       i!=m_Vars.end(); 
       ++i)
    RawSetVar(i->first, i->second);
}


void pqxx::connection_base::disconnect() throw ()
{
  dropconnect();
  if (m_Conn)
  {
    PQfinish(m_Conn);
    m_Conn = 0;
  }
}


bool pqxx::connection_base::is_open() const
{
  return m_Conn && (Status() == CONNECTION_OK);
}


PGSTD::auto_ptr<pqxx::noticer> 
pqxx::connection_base::set_noticer(PGSTD::auto_ptr<noticer> N)
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


void pqxx::connection_base::process_notice(const char msg[]) throw ()
{
  if (msg)
  {
    // TODO: Find cleaner solution for default case!
    if (m_Noticer.get()) (*m_Noticer.get())(msg);
    else fputs(msg, stderr);
  }
}


void pqxx::connection_base::trace(FILE *Out)
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
    const string LQ("LISTEN " + string(T->name())); 
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
      if (m_Conn && (R.second == ++R.first))
	PQexec(m_Conn, ("UNLISTEN " + string(T->name())).c_str());

      m_Triggers.erase(i);
    }
  }
  catch (const exception &e)
  {
    process_notice(e.what());
  }
}


void pqxx::connection_base::get_notifs()
{
  if (!is_open()) return;

  PQconsumeInput(m_Conn);

  // Even if somehow we receive notifications during our transaction, don't
  // deliver them.
  if (m_Trans.get()) return;

  for (PQAlloc<PGnotify> N( PQnotifies(m_Conn) ); N; N = PQnotifies(m_Conn))
  {
    typedef TriggerList::iterator TI;

    pair<TI, TI> Hit = m_Triggers.equal_range(string(N->relname));
    for (TI i = Hit.first; i != Hit.second; ++i)
      try
      {
        (*i->second)(N->be_pid);
      }
      catch (const exception &e)
      {
	process_notice("Exception in trigger handler '" +
		       i->first + 
		       "': " + 
		       e.what() +
		       "\n");
      }

    N.close();
  }
}


const char *pqxx::connection_base::ErrMsg() const
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

  // TODO: Is this really right?  What if there's just no memory?
  if (!R) throw broken_connection();
  R.CheckStatus(Query);
  get_notifs();
  return R;
}


void pqxx::connection_base::Reset()
{
  clear_fdmask();

  // Forget about any previously ongoing connection attempts
  dropconnect();

  // Attempt to set up or restore connection
  if (!m_Conn) 
  {
    Connect();
  }
  else 
  {
    PQreset(m_Conn);
    SetupState();
    clear_fdmask();
  }
}


void pqxx::connection_base::close() throw ()
{
  clear_fdmask();
  try
  {
    if (m_Trans.get()) 
      process_notice("Closing connection while transaction '" +
		     m_Trans.get()->name() +
		     "' still open\n");

    if (!m_Triggers.empty())
    {
      string T;
      for (TriggerList::const_iterator i = m_Triggers.begin(); 
	   i != m_Triggers.end();
	   ++i)
	T += " " + i->first;

        process_notice("Closing connection with outstanding triggers:" + 
	               T + 
		       "\n");
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


void pqxx::connection_base::InternalSetTrace()
{
  if (m_Trace) PQtrace(m_Conn, m_Trace);
  else PQuntrace(m_Conn);
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
    process_notice(string(e.what()) + "\n");
  }
}


void pqxx::connection_base::MakeEmpty(pqxx::result &R, ExecStatusType Stat)
{
  if (!m_Conn) 
    throw logic_error("libpqxx internal error: MakeEmpty() on null connection");

  R = result(PQmakeEmptyPGresult(m_Conn, Stat));
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
  char *Buf;
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

  if (!Result) 
  {
    Line.erase();
    if (PQendcopy(m_Conn)) throw runtime_error(ErrMsg());
  }
#endif

  return Result;
}


#include<iostream>// DEBUG CODE
bool pqxx::connection_base::WriteCopyLine(const string &Line, bool async)
{
  if (!is_open())
    throw logic_error("libpqxx internal error: "
	              "WriteCopyLine() without connection");

  bool OK, Result;
  int PutRes;
  const string L = Line + '\n';
  const char *const LC = L.c_str();
  const string::size_type Len = L.size();

#ifdef PQXX_HAVE_PQPUTCOPY
  if (async) go_async();
  PutRes = PQputCopyData(m_Conn, LC, Len);
  if (async) go_sync();
  OK = (PutRes != -1);
  Result = (PutRes != 0);
#else
  PutRes = PQputnbytes(m_Conn, LC, Len);
  OK = (PutRes != EOF);
  Result = true;
#endif
  if (!OK)
  {
    const string Msg = string("Error writing to table: ") + ErrMsg();
#ifdef PQXX_HAVE_PQPUTCOPY
    PQendcopy(m_Conn);
#endif
    throw runtime_error(Msg);
  }
  return Result;
}


void pqxx::connection_base::EndCopyWrite()
{
#ifdef PQXX_HAVE_PQPUTCOPY
  go_sync();
  int Res;
  do
  {
    Res = PQputCopyEnd(m_Conn, NULL);
    switch (Res)
    {
      case -1:
        throw runtime_error("Write to table failed: " + string(ErrMsg()));
      case 0:
        throw logic_error("libpqxx internal error: "
	    "table write is inexplicably asynchronous");
      case 1:
        for (result R(PQgetResult(m_Conn)); R; R=PQgetResult(m_Conn))
	    R.CheckStatus("[END COPY]");
        break;
      default:
        throw logic_error("libpqxx internal error: "
	    "unexpected result " + ToString(Res) + " from PQputCopyEnd()");
    }
  } while (!Res);
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


PGresult *pqxx::connection_base::get_result()
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

void pqxx::connection_base::wait_write() const
{
  const int fd = set_fdmask();
  select(fd+1, fdset_none, &m_fdmask, &m_fdmask, 0);
}


void pqxx::connection_base::go_sync()
{
  if (PQsetnonblocking(m_Conn, false)==-1)
    throw runtime_error("Return to blocking mode failed: "+string(ErrMsg()));
}

void pqxx::connection_base::go_async()
{
  if (PQsetnonblocking(m_Conn, true)==-1)
    throw runtime_error("Could not go to nonblocking mode: "+string(ErrMsg()));
}


