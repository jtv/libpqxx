/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connection.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::Connection class.
 *   pqxx::Connection encapsulates a frontend to backend connection
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_CONNECTION_H
#define PQXX_CONNECTION_H

#include <map>
#include <memory>
#include <stdexcept>

#include "pqxx/transactor.h"
#include "pqxx/util.h"


/* Use of the libpqxx library starts here.
 *
 * Everything that can be done with a database through libpqxx must go through
 * a Connection object.
 */

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


/// For internal use only
extern "C" void pqxxNoticeCaller(void *, const char *);

namespace pqxx
{
class in_doubt_error;	// See pqxx/transactionitf.h
class Result;
class TransactionItf;
class Trigger;

/// @deprecated Notice processor callback function.  Use Noticer.
extern "C" { typedef void (*NoticeProcessor)(void *arg, const char *msg); }

/// Base class for user-definable error/warning message processor
struct Noticer
{
  virtual ~Noticer() {}
  virtual void operator()(const char Msg[]) throw () =0;
};


/// Human-readable class names for use by Unique template.
template<> inline PGSTD::string Classname(const TransactionItf *) 
{ 
  return "TransactionItf"; 
}


/// Exception class for lost backend connection.
/** (May be changed once I find a standard exception class for this) */
class broken_connection : public PGSTD::runtime_error
{
public:
  broken_connection() : PGSTD::runtime_error("Connection to back end failed") {}
  explicit broken_connection(const PGSTD::string &whatarg) : 
    PGSTD::runtime_error(whatarg) {}
};


/// Connection class; represents a connection to a database.
/** This is the first class to look at when you wish to work with a database 
 * through libpqxx.  It is automatically opened by its constructor, and 
 * automatically closed upon destruction, if it hasn't already been closed 
 * manually.
 * To query or manipulate the database once connected, use one of the 
 * Transaction classes (see pqxx/transactionitf.h) or preferably the Transactor 
 * framework (see pqxx/transactor.h).
 */
class PQXX_LIBEXPORT Connection
{
public:
  /// Constructor.  Sets up connection based on PostgreSQL connection string.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.
   * @param Immediate if set, makes the connection immediately.  This is the
   * default.  If not set, the creation of the underlying database connection is
   * deferred until necessitated by actual use, and the Connection remains in a
   * deactivated state.  It will be activated transparently when calls to its
   * member functions make it necessary to do so.  This regime is referred to as
   * a "lazy" connection.
   */
  explicit Connection(const PGSTD::string &ConnInfo,
                      bool Immediate=true);				//[t1]

  /// Constructor.  Sets up connection based on PostgreSQL connection string.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  As a special
   * case, a null pointer is taken as the empty string.
   * @param Immediate if set, makes the connection immediately.  This is the
   * default.  If not set, the creation of the underlying database connection is
   * deferred until necessitated by actual use, and the Connection remains in a
   * deactivated state.  It will be activated transparently when calls to its
   * member functions make it necessary to do so.  This regime is referred to as
   * a "lazy" connection.
   */
  explicit Connection(const char ConnInfo[], bool Immediate=true);	//[t2]

  // /// Constructor.  Sets up a lazy connection with an empty connect string.
  // /** Note that this version of Connection is deferred by default, making it
  //  * easier to create connection pools where not every connection object is
  //  * going to be used.
  //  */
  // Connection();								//[t43]

  /// Destructor.  Implicitly closes the connection.
  ~Connection();							//[t1]

  /// Explicitly close connection.
  void Disconnect() const throw ();					//[t2]

  /// Is this connection open?
  bool is_open() const;							//[t1]

  /// Is this connection open? @deprecated Use is_open() instead
  bool IsOpen() const { return is_open(); }

  /// Perform the transaction defined by a Transactor-based object.
  template<typename TRANSACTOR> 
  void Perform(const TRANSACTOR &, int Attempts=3);			//[t4]

  /// Set callback method for postgresql status output. @deprecated Use Noticer.
  /** return value is the previous handler.  Passing a NULL callback pointer 
   * simply returns the existing callback.  The callback must have C linkage.
   */
  NoticeProcessor SetNoticeProcessor(NoticeProcessor, void *arg);	//[t1]

  /// Set handler for postgresql errors or warning messages.
  /** Return value is the previous handler.  Must not be NULL.
   */
  std::auto_ptr<Noticer> SetNoticer(std::auto_ptr<Noticer>);
  Noticer *GetNoticer() const throw () { return m_Noticer.get(); }

  /// Invoke notice processor function.  The message should end in newline.
  void ProcessNotice(const char[]) throw ();				//[t1]
  /// Invoke notice processor function.  The message should end in newline.
  void ProcessNotice(const PGSTD::string &msg) throw () 		//[t1]
  	{ ProcessNotice(msg.c_str()); }

  /// Enable tracing to a given output stream, or NULL to disable.
  void Trace(FILE *);							//[t3]
  /// Disable tracing.  @deprecated Use Trace(0).
  void Untrace() { Trace(0); }


  /// Check for pending trigger notifications and take appropriate action.
  void GetNotifs();							//[t4]

  // Miscellaneous query functions (probably not needed very often)
 
  /// Name of database we're connected to, if any.
  const char *DbName() const throw ()					//[t1]
  	{ Activate(); return PQdb(m_Conn); }

  /// Database user ID we're connected under, if any.
  const char *UserName() const throw ()					//[t1]
  	{ Activate(); return  PQuser(m_Conn); }

  /// Address of server (NULL for local connections).
  const char *HostName() const throw ()					//[t1]
  	{ Activate(); return PQhost(m_Conn); }

  /// Server port number we're connected to.
  const char *Port() const throw () 					//[t1]
  	{ Activate(); return PQport(m_Conn); }

  /// Full connection string as used to set up this connection.
  const char *Options() const throw () 					//[t1]
  	{ return m_ConnInfo.c_str(); }

  /// Process ID for backend process.
  /** Use with care: connections may be lost and automatically re-established
   * without your knowledge, in which case this process ID may no longer be
   * correct.  You may, however, assume that this number remains constant and
   * reliable within the span of a successful backend transaction.  If the
   * transaction fails, which may be due to a lost connection, then this number
   * will have become invalid at some point within the transaction.
   */
  int BackendPID() const						//[t1]
    	{ return m_Conn ? PQbackendPID(m_Conn) : 0; }

  /// Explicitly activate deferred or deactivated connection.
  /** Use of this method is entirely optional.  Whenever a Connection is used
   * while in a deferred or deactivated state, it will transparently try to
   * bring itself into an actiaveted state.  This function is best viewed as an
   * explicit hint to the Connection that "if you're not in an active state, now
   * would be a good time to get into one."  Whether a Connection is currently
   * in an active state or not makes no real difference to its functionality.
   * There is also no particular need to match calls to Activate() with calls to
   * Deactivate().  A good time to call Activate() might be just before you
   * first open a transaction on a lazy connection.
   */
  void Activate() const { if (!m_Conn) Connect(); }			//[]

  /// Explicitly deactivate connection.
  /** Like its counterpart Activate(), this method is entirely optional.  
   * Calling this function really only makes sense if you won't be using this
   * Connection for a while and want to reduce the number of open connections on
   * the database server.
   * There is no particular need to match or pair calls to Deactivate() with
   * calls to Activate(), but calling Deactivate() during a transaction is an
   * error.
   */
  void Deactivate() const;						//[]

private:
  void Connect() const;
  void SetupState() const;
  void InternalSetTrace() const;
  int Status() const { return PQstatus(m_Conn); }
  const char *ErrMsg() const;
  void Reset(const char OnReconnect[]=0);

  PGSTD::string m_ConnInfo;	/// Connection string
  mutable PGconn *m_Conn;	/// Connection handle
  Unique<TransactionItf> m_Trans;/// Active transaction on connection, if any

  /// Client-set notice processor function
  mutable NoticeProcessor m_NoticeProcessor;
  void *m_NoticeProcessorArg;	// Client-set argument to notice processor func
  std::auto_ptr<Noticer> m_Noticer;
  FILE *m_Trace;		// File to trace to, if any

  typedef PGSTD::multimap<PGSTD::string, pqxx::Trigger *> TriggerList;
  TriggerList m_Triggers;

  friend class TransactionItf;
  Result Exec(const char[], int Retries=3, const char OnReconnect[]=0);
  void RegisterTransaction(const TransactionItf *);
  void UnregisterTransaction(const TransactionItf *) throw ();
  void MakeEmpty(Result &, ExecStatusType=PGRES_EMPTY_QUERY);
  void BeginCopyRead(const PGSTD::string &Table);
  bool ReadCopyLine(PGSTD::string &);
  void BeginCopyWrite(const PGSTD::string &Table);
  void WriteCopyLine(const PGSTD::string &);
  void EndCopy();

  friend class Trigger;
  void AddTrigger(Trigger *);
  void RemoveTrigger(Trigger *) throw ();

  // Not allowed:
  Connection(const Connection &);
  Connection &operator=(const Connection &);
};



/** Invoke a Transactor, making at most Attempts attempts to perform the
 * encapsulated code on the database.  If the code throws any exception other
 * than broken_connection, it will be aborted right away.
 * Take care: neither OnAbort() nor OnCommit() will be invoked on the original
 * transactor you pass into the function.  It only serves as a prototype for
 * the transaction to be performed.  In fact, this function may copy-construct
 * any number of Transactors from the one you passed in, calling either 
 * OnCommit() or OnAbort() only on those that actually have their operator()
 * invoked.
 */
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
      typename TRANSACTOR::argument_type X(*this, T2.Name());
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
      // Don't try to forge ahead if we don't even know what happened
      T2.OnAbort("Unknown exception");
      throw;
    }

    T2.OnCommit();
  } while (!Done);
}


}

#endif

