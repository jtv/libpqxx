/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connection_base.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::Connection_base abstract base class.
 *   pqxx::Connection_base encapsulates a frontend to backend connection
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_CONNECTION_BASE_H
#define PQXX_CONNECTION_BASE_H

#include <map>
#include <memory>

#include "pqxx/except.h"
#include "pqxx/transactor.h"
#include "pqxx/util.h"


/* Use of the libpqxx library starts here.
 *
 * Everything that can be done with a database through libpqxx must go through
 * a connection object derived from Connection_base.
 */

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */

namespace pqxx
{
class Result;
class Transaction_base;
class Trigger;

/// Base class for user-definable error/warning message processor
/** To define a custom method of handling notices, derive a new class from
 * Noticer and override the virtual function "operator()(const char[]) throw()"
 * to process the message passed to it.
 */
struct PQXX_LIBEXPORT Noticer
{
  virtual ~Noticer() {}
  virtual void operator()(const char Msg[]) throw () =0;
};


/// Human-readable class names for use by Unique template.
template<> inline PGSTD::string Classname(const Transaction_base *) 
{ 
  return "Transaction_base"; 
}


/// Connection_base abstract base class; represents a connection to a database.
/** This is the first class to look at when you wish to work with a database 
 * through libpqxx.  Depending on the implementing concrete child class, a
 * connection can be automatically opened when it is constructed, or when it is
 * first used.  The connection is automatically closed upon destruction, if it 
 * hasn't already been closed manually.
 * To query or manipulate the database once connected, use one of the 
 * Transaction classes (see pqxx/transaction_base.h) or preferably the 
 * Transactor framework (see pqxx/transactor.h).
 * A word of caution: if a network connection to the database server fails, the
 * connection will be restored automatically (although any transaction going on
 * at the time will have to be aborted).  This also means that any information
 * set in previous transactions that is not stored in the database, such as
 * connection-local variables defined with PostgreSQL's SET command, will be
 * lost.  Whenever you need to create such state, either do it within each
 * transaction that may need it, or use specialized functions made available by
 * libpqxx.  Always avoid raw queries if libpqxx offers a dedicated function for
 * the same purpose.
 */
class PQXX_LIBEXPORT Connection_base
{
public:
  /// Constructor.  Sets up connection PostgreSQL connection string.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.
   */
  explicit Connection_base(const PGSTD::string &ConnInfo);		//[t2]

  /// Constructor.  Sets up connection based on PostgreSQL connection string.
  /** @param ConnInfo a PostgreSQL connection string specifying any required
   * parameters, such as server, port, database, and password.  As a special
   * case, a null pointer is taken as the empty string.
   */
  explicit Connection_base(const char ConnInfo[]);			//[t2]

  /// Destructor.  Implicitly closes the connection.
  virtual ~Connection_base() =0;						//[t1]

  /// Explicitly close connection.
  void Disconnect() throw ();					//[t2]

  /// Is this connection open?
  bool is_open() const;							//[t1]

  /// Perform the transaction defined by a Transactor-based object.
  /** The function may create and execute several copies of the Transactor
   * before it succeeds.  If there is any doubt over whether it succeeded
   * (this can happen if the connection is lost just before the backend can
   * confirm success), it is no longer retried and an error message is
   * generated.
   * @param T the Transactor to be executed.
   * @param Attempts the maximum number of attempts to be made to execute T.
   */
  template<typename TRANSACTOR> 
  void Perform(const TRANSACTOR &T, int Attempts=3);			//[t4]

  // TODO: Define a default Noticer (mainly to help out Windows users)
  /// Set handler for postgresql errors or warning messages.
  /** Return value is the previous handler.  Ownership of any previously set 
   * Noticer is also passed to the caller, so unless it is stored in another 
   * auto_ptr, it will be deleted from the caller's context.  
   * This may be important when running under Windows, where a DLL cannot free 
   * memory allocated by the main program.
   * If a Noticer is set when the Connection_base is destructed, it will also be
   * deleted.
   * @param N the new message handler; must not be null or equal to the old one
   */
  PGSTD::auto_ptr<Noticer> SetNoticer(PGSTD::auto_ptr<Noticer> N);	//[t14]
  Noticer *GetNoticer() const throw () { return m_Noticer.get(); }	//[t14]

  /// Invoke notice processor function.  The message should end in newline.
  void ProcessNotice(const char[]) throw ();				//[t14]
  /// Invoke notice processor function.  The message should end in newline.
  void ProcessNotice(const PGSTD::string &msg) throw () 		//[t14]
  	{ ProcessNotice(msg.c_str()); }

  /// Enable tracing to a given output stream, or NULL to disable.
  void Trace(FILE *);							//[t3]

  /// Check for pending trigger notifications and take appropriate action.
  void GetNotifs();							//[t4]

  // Miscellaneous query functions (probably not needed very often)
 
  /// Name of database we're connected to, if any.
  const char *DbName()							//[t1]
  	{ Activate(); return PQdb(m_Conn); }

  /// Database user ID we're connected under, if any.
  const char *UserName()						//[t1]
  	{ Activate(); return  PQuser(m_Conn); }

  /// Address of server (NULL for local connections).
  const char *HostName()						//[t1]
  	{ Activate(); return PQhost(m_Conn); }

  /// Server port number we're connected to.
  const char *Port()		 					//[t1]
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
  /** Use of this method is entirely optional.  Whenever a connection is used
   * while in a deferred or deactivated state, it will transparently try to
   * bring itself into an actiaveted state.  This function is best viewed as an
   * explicit hint to the connection that "if you're not in an active state, now
   * would be a good time to get into one."  Whether a connection is currently
   * in an active state or not makes no real difference to its functionality.
   * There is also no particular need to match calls to Activate() with calls to
   * Deactivate().  A good time to call Activate() might be just before you
   * first open a transaction on a lazy connection.
   */
  void Activate() { if (!m_Conn) Connect(); }				//[t12]

  /// Explicitly deactivate connection.
  /** Like its counterpart Activate(), this method is entirely optional.  
   * Calling this function really only makes sense if you won't be using this
   * connection for a while and want to reduce the number of open connections on
   * the database server.
   * There is no particular need to match or pair calls to Deactivate() with
   * calls to Activate(), but calling Deactivate() during a transaction is an
   * error.
   */
  void Deactivate();							//[t12]

  /// Set client-side character encoding
  /** Search the PostgreSQL documentation for "multibyte" or "character set
   * encodings" to find out more about the available encodings, how to extend
   * them, and how to use them.  Not all server-side encodings are compatible 
   * with all client-side encodings or vice versa.
   * @param Encoding name of the character set encoding to use
   */
  void SetClientEncoding(const PGSTD::string &Encoding)			//[t7]
  	{ SetVariable("CLIENT_ENCODING", Encoding); }

  /// Set session variable
  /** Set a session variable for this connection, using the SET command.  If the
   * connection to the database is lost and recovered, the last-set value will
   * be restored automatically.  See the PostgreSQL documentation for a list of
   * variables that can be set and their permissible values.
   * If a transaction is currently in progress, aborting that transaction will
   * normally discard the newly set value.  Known exceptions are NonTransaction
   * (which doesn't start a real backend transaction) and PostgreSQL versions
   * prior to 7.3.
   * @param Var variable to set
   * @param Value value to set, which may be an identifier, a quote string, etc.
   */
  void SetVariable(const PGSTD::string &Var, const PGSTD::string &Value);//[]

protected:
  /// To be used by implementation classes: really connect to database
  void Connect();

private:
  void SetupState();
  void InternalSetTrace();
  int Status() const { return PQstatus(m_Conn); }
  const char *ErrMsg() const;
  void Reset(const char OnReconnect[]=0);
  void close() throw ();
  void RestoreVars();

  PGSTD::string m_ConnInfo;	/// Connection string
  PGconn *m_Conn;	/// Connection handle
  Unique<Transaction_base> m_Trans;/// Active transaction on connection, if any

  PGSTD::auto_ptr<Noticer> m_Noticer;	/// User-defined notice processor
  FILE *m_Trace;		/// File to trace to, if any

  typedef PGSTD::multimap<PGSTD::string, pqxx::Trigger *> TriggerList;
  TriggerList m_Triggers;	/// Triggers client is listening on

  PGSTD::map<PGSTD::string, PGSTD::string> m_Vars; /// Variables set in session

  friend class Transaction_base;
  Result Exec(const char[], int Retries=3, const char OnReconnect[]=0);
  void RegisterTransaction(Transaction_base *);
  void UnregisterTransaction(Transaction_base *) throw ();
  void MakeEmpty(Result &, ExecStatusType=PGRES_EMPTY_QUERY);
  void BeginCopyRead(const PGSTD::string &Table);
  bool ReadCopyLine(PGSTD::string &);
  void BeginCopyWrite(const PGSTD::string &Table);
  void WriteCopyLine(const PGSTD::string &);
  void EndCopy();
  void RawSetVar(const PGSTD::string &Var, const PGSTD::string &Value);
  void AddVariables(const PGSTD::map<PGSTD::string, PGSTD::string> &);

  friend class LargeObject;
  PGconn *RawConnection() const { return m_Conn; }

  friend class Trigger;
  void AddTrigger(Trigger *);
  void RemoveTrigger(Trigger *) throw ();

  // Not allowed:
  Connection_base(const Connection_base &);
  Connection_base &operator=(const Connection_base &);
};


}


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
inline void pqxx::Connection_base::Perform(const TRANSACTOR &T,
                                         int Attempts)
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


// Put this here so on Windows, any Noticer will be deleted in caller's context
inline pqxx::Connection_base::~Connection_base()
{
  close();
}

#endif

