/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_connection.h
 *
 *   DESCRIPTION
 *      definition of the Pg::Connection class.
 *   Pg::Connection encapsulates a frontend to backend connection
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_CONNECTION_H
#define PG_CONNECTION_H

#include <map>
#include <stdexcept>

#include "pg_transactor.h"
#include "pg_util.h"

/* Use of the libpqxx library starts here.
 *
 * Everything that can be done with a database through libpqxx must go through
 * a Connection object.
 */

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

// TODO: Teach Transactor to handle multiple connections--quasi "2-phase commit"


namespace Pg
{
class in_doubt_error;	// See pg_transactionitf.h
class Result;		// See pg_result.h
class TransactionItf;	// See pg_transactionitf.h
class Trigger;		// See pg_trigger.h

extern "C" { typedef void (*NoticeProcessor)(void *arg, const char *msg); }


template<> inline PGSTD::string Classname(const TransactionItf *) 
{ 
  return "TransactionItf"; 
}


// Exception class for lost backend connection
// (May be changed once I find a standard exception class for this)
class broken_connection : public PGSTD::runtime_error
{
public:
  broken_connection() : PGSTD::runtime_error("Connection to back end failed") {}
  explicit broken_connection(const PGSTD::string &whatarg) : 
    PGSTD::runtime_error(whatarg) {}
};


// Connection class.  This is the first class to look at when you wish to work
// with a database through libpqxx.  It is automatically opened by its
// constructor, and automatically closed upon destruction, if it hasn't already
// been closed manually.
// To query or manipulate the database once connected, use the Transaction
// class (see pg_transaction.h) or preferably the Transactor framework (see 
// pg_transactor.h).
class Connection
{
public:
  // TODO: Allow two-stage creation (create first, open later)
  explicit Connection(const PGSTD::string &ConnInfo);			//[t1]
  ~Connection();							//[t1]

  void Disconnect() throw ();						//[t2]

  bool IsOpen() const;							//[t1]

  template<typename TRANSACTOR> 
  void Perform(const TRANSACTOR &, int Attempts=3);			//[t4]

  // Set callback method for postgresql status output; return value is previous
  // handler.  Passing a NULL callback pointer simply returns the existing
  // callback.  The callback must have C linkage.
  NoticeProcessor SetNoticeProcessor(NoticeProcessor, void *arg);	//[t1]

  // Invoke notice processor function.  The message should end in newline.
  void ProcessNotice(const char[]) throw ();				//[t1]
  void ProcessNotice(PGSTD::string msg) throw () 			//[t1]
  	{ ProcessNotice(msg.c_str()); }

  // Enable/disable tracing to a given output stream
  void Trace(FILE *);							//[t3]
  void Untrace();							//[t3]


  // Check for pending trigger notifications and take appropriate action
  void GetNotifs();							//[t4]

  // Miscellaneous query functions (probably not needed very often)
 
  // Name of database we're connected to
  const char *DbName() const throw ()					//[t1]
  	{ return m_Conn ? PQdb(m_Conn) : ""; }

  // Database user ID we're connected under
  const char *UserName() const throw ()					//[t1]
  	{ return m_Conn ? PQuser(m_Conn) : ""; }

  // Address of server (NULL for local connections)
  const char *HostName() const throw ()					//[t1]
  	{ return m_Conn ? PQhost(m_Conn) : ""; }

  // Server port number we're connected to
  const char *Port() const throw () 					//[t1]
  	{ return m_Conn ? PQport(m_Conn) : ""; }

  // Full connection string
  const char *Options() const throw () 					//[t1]
  	{ return m_ConnInfo.c_str(); }

  // Process ID for backend process
  int BackendPID() const;						//[t1]

private:
  void Connect();
  int Status() const { return PQstatus(m_Conn); }
  const char *ErrMsg() const;
  void Reset(const char *OnReconnect=0);

  PGSTD::string m_ConnInfo;	// Connection string
  PGconn *m_Conn;		// Connection handle
  Unique<TransactionItf> m_Trans;// Active transaction on connection, if any
  void *m_NoticeProcessorArg;	// Client-set argument to notice processor func

  // TODO: Use multimap when gcc supports them!
  typedef PGSTD::map<PGSTD::string, Pg::Trigger *> TriggerList;
  TriggerList m_Triggers;

  friend class TransactionItf;
  Result Exec(const char[], int Retries=3, const char OnReconnect[]=0);
  void RegisterTransaction(const TransactionItf *);
  void UnregisterTransaction(const TransactionItf *) throw ();
  void MakeEmpty(Result &, ExecStatusType=PGRES_EMPTY_QUERY);
  void BeginCopyRead(PGSTD::string Table);
  bool ReadCopyLine(PGSTD::string &);
  void BeginCopyWrite(PGSTD::string Table);
  void WriteCopyLine(PGSTD::string);
  void EndCopy();

  friend class Trigger;
  void AddTrigger(Trigger *);
  void RemoveTrigger(const Trigger *) throw ();

  // Not allowed:
  Connection(const Connection &);
  Connection &operator=(const Connection &);
};



// Invoke a Transactor, making at most Attempts attempts to perform the
// encapsulated code on the database.  If the code throws any exception other
// than broken_connection, it will be aborted right away.
// Take care: neither OnAbort() nor OnCommit() will be invoked on the original
// transactor you pass into the function.  It only serves as a prototype for
// the transaction to be performed.  In fact, this function may copy-construct
// any number of Transactors from the one you passed in, calling either 
// OnCommit() or OnAbort() only on those that actually have their operator()
// invoked.
template<typename TRANSACTOR> 
inline void Connection::Perform(const TRANSACTOR &T,
                                int Attempts)				//[t4]
{
  if (Attempts <= 0) return;

  bool Done = false;

  // Make attempts to perform T
  // TODO: Differentiate between db-related exceptions and other exceptions?
  do
  {
    --Attempts;

    // Work on a copy of T2 so we can restore the starting situation if need be
    TRANSACTOR T2(T);
    try
    {
      typename TRANSACTOR::TRANSACTIONTYPE X(*this, T2.Name());
      T2(X);
      X.Commit();
      Done = true;
    }
    catch (const in_doubt_error &)
    {
      // Not sure whether transaction went through or not.  The last thing in
      // the world that we should do now is retry.
      T2.OnDoubt();
      throw;
    }
    catch (const PGSTD::exception &e)
    {
      // Could be any kind of error.  
      T2.OnAbort(e.what());
      if (Attempts <= 0) throw;
      continue;
    }
    catch (...)
    {
      // This shouldn't happen!  If it does, don't try to forge ahead
      T2.OnAbort("Unknown exception");
      throw;
    }

    // Commit has to happen inside loop where T2 is still in scope.
    if (!Done) 
      throw PGSTD::logic_error("Internal libpqxx error: Pg::Connection: "
		             "broken Perform() loop");

    T2.OnCommit();
  } while (!Done);
}


}

#endif

