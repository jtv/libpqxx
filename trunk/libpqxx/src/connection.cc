/*-------------------------------------------------------------------------
 *
 *   FILE
 *	connection.cc
 *
 *   DESCRIPTION
 *      implementation of the pqxx::Connection class.
 *   pqxx::Connection encapsulates a frontend to backend connection
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pqxx/connection.h"
#include "pqxx/result.h"
#include "pqxx/transaction.h"
#include "pqxx/trigger.h"


using namespace PGSTD;

namespace
{

// Keep track of a pointer to be free()d automatically.
// Ownership policy is simple: object dies when CAlloc object's value does.
template<typename T> class CAlloc
{
  T *m_Obj;
public:
  CAlloc() : m_Obj(0) {}
  explicit CAlloc(T *obj) : m_Obj(obj) {}
  ~CAlloc() { close(); }

  CAlloc &operator=(T *obj) throw ()
  { 
    if (obj != m_Obj)
    {
      close();
      m_Obj = obj;
    }
    return *this;
  }

  operator bool() const throw () { return m_Obj != 0; }
  bool operator!() const throw () { return !m_Obj; }

  T *operator->() const
  {
    if (!m_Obj) throw logic_error("Null pointer dereferenced");
    return m_Obj;
  }

  T &operator*() const { return *operator->(); }

  T *c_ptr() const throw () { return m_Obj; }

  void close() throw () { if (m_Obj) free(m_Obj); m_Obj = 0; }

private:
  CAlloc(const CAlloc &);		// Not allowed
  CAlloc &operator=(const CAlloc &);	// Not allowed
};
}


pqxx::Connection::Connection(const string &ConnInfo) :
  m_ConnInfo(ConnInfo),
  m_Conn(0),
  m_Trans()
{
  Connect();
}


pqxx::Connection::~Connection()
{
  if (m_Trans.get()) 
    ProcessNotice("Closing connection while transaction '" +
		  m_Trans.get()->Name() +
		  "' still open\n");

  if (!m_Triggers.empty())
  {
    try
    {
      string T;
      for (TriggerList::const_iterator i = m_Triggers.begin(); 
	   i != m_Triggers.end();
	   ++i)
	T += " " + i->first;

      ProcessNotice("Closing connection with outstanding triggers:" + T);
    }
    catch (...)
    {
    }

    m_Triggers.clear();
  }

  Disconnect();
}


int pqxx::Connection::BackendPID() const
{
  if (!m_Conn) throw runtime_error("No connection");

  return PQbackendPID(m_Conn);
}


void pqxx::Connection::Connect()
{
  Disconnect();
  m_Conn = PQconnectdb(m_ConnInfo.c_str());

  if (!m_Conn)
    throw broken_connection();

  if (!IsOpen())
  {
    const string Msg( ErrMsg() );
    Disconnect();
    throw broken_connection(Msg);
  }

  if (Status() != CONNECTION_OK)
  {
    const string Msg( ErrMsg() );
    Disconnect();
    throw runtime_error(Msg);
  }
}


void pqxx::Connection::Disconnect() throw ()
{
  if (m_Conn)
  {
    PQfinish(m_Conn);
    m_Conn = 0;
  }
}


bool pqxx::Connection::IsOpen() const
{
  return m_Conn && (Status() != CONNECTION_BAD);
}


pqxx::NoticeProcessor 
pqxx::Connection::SetNoticeProcessor(pqxx::NoticeProcessor NewNP,
		                   void *arg)
{
  m_NoticeProcessorArg = arg;
  return PQsetNoticeProcessor(m_Conn, NewNP, arg);
}


void pqxx::Connection::ProcessNotice(const char msg[]) throw ()
{
  if (msg) (*(SetNoticeProcessor(0,0)))(m_NoticeProcessorArg, msg);
}


void pqxx::Connection::Trace(FILE *Out)
{
  PQtrace(m_Conn, Out);
}


void pqxx::Connection::Untrace()
{
  PQuntrace(m_Conn);
}


void pqxx::Connection::AddTrigger(pqxx::Trigger *T)
{
  if (!T) throw invalid_argument("Null trigger registered");

  // Add to triggers list and attempt to start listening.
  if (m_Triggers.insert(make_pair(T->Name(), T)).second)
  {
    Result R( PQexec(m_Conn, ("LISTEN " + string(T->Name())).c_str()) );

    try
    {
      R.CheckStatus();
    }
    catch (const broken_connection &)
    {
    }
    catch (const exception &)
    {
      if (IsOpen()) throw;
    }
  }
}


void pqxx::Connection::RemoveTrigger(const pqxx::Trigger *T) throw ()
{
  if (!T) return;

  try
  {
    const string TName = T->Name();
    const TriggerList::iterator l = m_Triggers.lower_bound(TName),
                                u = m_Triggers.upper_bound(TName);

    TriggerList::iterator i;
    for (i = l; (i != u) && (i->second != T); ++i);

    if (i == u) 
      ProcessNotice("Attempt to remove unknown trigger '" + 
		    string(TName) + 
		    "'");
    else
      m_Triggers.erase(i);
  }
  catch (const exception &e)
  {
    ProcessNotice(e.what());
  }
}


void pqxx::Connection::GetNotifs()
{
  typedef TriggerList::iterator TI;

  PQconsumeInput(m_Conn);

  // Even if somehow we receive notifications during our transaction, don't
  // deliver them.
  if (m_Trans.get()) return;

  for (CAlloc<PGnotify> N( PQnotifies(m_Conn)); N; N = PQnotifies(m_Conn))
  {
    pair<TI, TI> Hit = m_Triggers.equal_range(string(N->relname));
    for (TI i = Hit.first; i != Hit.second; ++i)
      try
      {
        (*i->second)(N->be_pid);
      }
      catch (const exception &e)
      {
	ProcessNotice("Exception in trigger handler: " + 
		      string(e.what()) +
		      "\n");
      }
  }
}


const char *pqxx::Connection::ErrMsg() const
{
  return m_Conn ? PQerrorMessage(m_Conn) : "No connection to database";
}


pqxx::Result pqxx::Connection::Exec(const char Q[], 
                                int Retries, 
				const char OnReconnect[])
{
  if (!m_Conn)
    throw runtime_error("No connection to database");

  Result R( PQexec(m_Conn, Q) );

  while ((Retries > 0) && !R && !IsOpen())
  {
    Retries--;

    Reset(OnReconnect);
    if (IsOpen()) R = PQexec(m_Conn, Q);
  }

  if (!R) throw broken_connection();
  R.CheckStatus();

  GetNotifs();

  return R;
}


void pqxx::Connection::Reset(const char OnReconnect[])
{
  // Attempt to restore connection
  PQreset(m_Conn);

  // Reinstate all active triggers
  try
  {
    const TriggerList::const_iterator End = m_Triggers.end();
    string Last;
    for (TriggerList::const_iterator i = m_Triggers.begin(); i != End; ++i)
    {
      // m_Triggers is supposed to be able to handle multiple Triggers waiting
      // on the same event; issue just one LISTEN for each event.
      // TODO: Change TriggerList to be a multimap once compiler supports it
      if (i->first != Last)
      {
        Result R( PQexec(m_Conn, ("LISTEN " + i->first).c_str()) );
        R.CheckStatus();
	Last = i->first;
      }
    }

    // Perform any extra patchup work involved in restoring the connection,
    // typically set up a transaction.
    if (OnReconnect)
    {
      Result Temp( PQexec(m_Conn, OnReconnect) );
      Temp.CheckStatus();
    }
  }
  catch (...)
  {
    if (IsOpen()) throw;
  }
}


void pqxx::Connection::RegisterTransaction(const TransactionItf *T)
{
  m_Trans.Register(T);
}


void pqxx::Connection::UnregisterTransaction(const TransactionItf *T) throw ()
{
  try
  {
    m_Trans.Unregister(T);
  }
  catch (const exception &e)
  {
    ProcessNotice(string(e.what()) + "\n");
  }
}


void pqxx::Connection::MakeEmpty(pqxx::Result &R, ExecStatusType Stat)
{
  R = Result(PQmakeEmptyPGresult(m_Conn, Stat));
}


void pqxx::Connection::BeginCopyRead(string Table)
{
  Result R( Exec(("COPY " + Table + " TO STDOUT").c_str()) );
  R.CheckStatus();
}


bool pqxx::Connection::ReadCopyLine(string &Line)
{
  char Buf[256];
  bool LineComplete = false;

  Line.erase();

  while (!LineComplete)
  {
    switch (PQgetline(m_Conn, Buf, sizeof(Buf)))
    {
    case EOF:
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

  bool Result = (Line != "\\.");

  if (!Result) Line.erase();

  return Result;
}


void pqxx::Connection::BeginCopyWrite(string Table)
{
  Result R( Exec(("COPY " + Table + " FROM STDIN").c_str()) );
  R.CheckStatus();
}



void pqxx::Connection::WriteCopyLine(string Line)
{
  PQputline(m_Conn, (Line + "\n").c_str());
}


// End COPY operation.  Careful: this assumes that no more lines remain to be
// read or written, and the COPY operation has been properly terminated with a
// line containing only the two characters "\."
void pqxx::Connection::EndCopy()
{
  if (PQendcopy(m_Conn) != 0) throw runtime_error(ErrMsg());
}


