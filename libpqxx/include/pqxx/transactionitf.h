/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transactionitf.h
 *
 *   DESCRIPTION
 *      common code and definitions for the transaction classes.
 *   pqxx::TransactionItf defines the interface for any abstract class that
 *   represents a database transaction
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TRANSACTIONITF_H
#define PQXX_TRANSACTIONITF_H


/* End-user programs need not include this file, unless they define their own
 * transaction classes.  This is not something the typical program should want
 * to do.
 *
 * However, reading this file is worthwhile because it defines the public
 * interface for the available transaction classes such as Transaction and 
 * NonTransaction.
 */


#include "pqxx/connectionitf.h"
#include "pqxx/result.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{
class ConnectionItf; 	// See pqxx/connectionitf.h
class Result; 		// See pqxx/result.h
class TableStream;	// See pqxx/tablestream.h


template<> inline PGSTD::string Classname(const TableStream *) 
{ 
  return "TableStream"; 
}


/// Interface definition (and common code) for "transaction" classes.  
/** All database access must be channeled through one of these classes for 
 * safety, although not all implementations of this interface need to provide 
 * full transactional integrity.
 *
 * Several implementations of this interface are shipped with libpqxx, including
 * the plain Transaction class, the entirely unprotected NonTransaction, and the
 * more cautions RobustTransaction.
 */
class PQXX_LIBEXPORT TransactionItf
{
public:
  virtual ~TransactionItf() =0;						//[t1]

  /// Commit the transaction
  /** Unless this function is called explicitly, the transaction will not be
   * committed (actually the NonTransaction implementation breaks this rule,
   * hence the name).
   *
   * Once this function returns, the whole transaction will typically be
   * irrevocably completed in the database.  There is also, however, a minute
   * risk that the connection to the database may be lost at just the wrong
   * moment.  In that case, libpqxx may be unable to determine whether the
   * transaction was completed or aborted and an in_doubt_error will be thrown
   * to make this fact known to the caller.  The RobustTransaction 
   * implementation takes some special precautions to reduce this risk.
   */
  void Commit();							//[t1]

  /// Abort the transaction
  /** No special effort is required to call this function; it will be called
   * implicitly when the transaction is destructed.
   */
  void Abort();								//[t10]

  /// Execute query
  /** Perform a query in this transaction.
   * @param Query the query or command to execute
   * @param Desc optional identifier for query, to help pinpoint SQL errors
   */
  Result Exec(const char Query[], 
      	      const PGSTD::string &Desc=PGSTD::string());		//[t1]

  /// Execute query
  /** Perform a query in this transaction.  This version may be slightly
   * slower than the version taking a const char[], although the difference is
   * not likely to be very noticeable compared to the time required to execute
   * even a simple query.
   * @param Query the query or command to execute
   * @param Desc optional identifier for query, to help pinpoint SQL errors
   */
  Result Exec(const PGSTD::string &Query,
              const PGSTD::string &Desc=PGSTD::string()) 		//[t2]
  	{ return Exec(Query.c_str(), Desc); }

  /// Have connection process warning message
  void ProcessNotice(const char Msg[]) { m_Conn.ProcessNotice(Msg); }	//[t14]
  /// Have connection process warning message
  void ProcessNotice(const PGSTD::string &Msg) 				//[t14]
  	{ m_Conn.ProcessNotice(Msg); }

  PGSTD::string Name() const { return m_Name; }				//[t1]

  /// Connection this transaction is running in
  ConnectionItf &Conn() const { return m_Conn; }			//[]

protected:
  /// Create a transaction.  The optional name, if given, must begin with a
  /// letter and may contain letters and digits only.
  explicit TransactionItf(ConnectionItf &, 
		          const PGSTD::string &TName=PGSTD::string());

  /// Begin transaction.  To be called by implementing class, typically from 
  /// constructor.
  void Begin();

  /// End transaction.  To be called by implementing class' destructor 
  void End() throw ();

  /// To be implemented by derived implementation class.
  virtual void DoBegin() =0;
  virtual Result DoExec(const char Query[]) =0;
  virtual void DoCommit() =0;
  virtual void DoAbort() =0;

  // For use by implementing class:

  /// Execute query on connection directly
  Result DirectExec(const char C[], int Retries, const char OnReconnect[]);
 
private:
  /* A Transaction goes through the following stages in its lifecycle:
   *  - nascent: the transaction hasn't actually begun yet.  If our connection 
   *    fails at this stage, it may recover and the Transaction can attempt to
   *    establish itself again.
   *  - active: the transaction has begun.  Since no commit command has been 
   *    issued, abortion is implicit if the connection fails now.
   *  - aborted: an abort has been issued; the transaction is terminated and 
   *    its changes to the database rolled back.  It will accept no further 
   *    commands.
   *  - committed: the transaction has completed successfully, meaning that a 
   *    commit has been issued.  No further commands are accepted.
   *  - in_doubt: the connection was lost at the exact wrong time, and there is
   *    no way of telling whether the transaction was committed or aborted.
   *
   * Checking and maintaining state machine logic is the responsibility of the 
   * base class (ie., this one).
   */
  enum Status 
  { 
    st_nascent, 
    st_active, 
    st_aborted, 
    st_committed,
    st_in_doubt
  };


  friend class Cursor;
  int GetUniqueCursorNum() { return m_UniqueCursorNum++; }
  void MakeEmpty(Result &R) const { m_Conn.MakeEmpty(R); }

  friend class TableStream;
  void RegisterStream(const TableStream *);
  void UnregisterStream(const TableStream *) throw ();
  void EndCopy() { m_Conn.EndCopy(); }
  friend class TableReader;
  void BeginCopyRead(const PGSTD::string &Table) 
  	{ m_Conn.BeginCopyRead(Table); }
  bool ReadCopyLine(PGSTD::string &L) { return m_Conn.ReadCopyLine(L); }
  friend class TableWriter;
  void BeginCopyWrite(const PGSTD::string &Table) 
  	{ m_Conn.BeginCopyWrite(Table); }
  void WriteCopyLine(const PGSTD::string &L) { m_Conn.WriteCopyLine(L); }

  ConnectionItf &m_Conn;

  PGSTD::string m_Name;
  int m_UniqueCursorNum;
  Unique<TableStream> m_Stream;
  Status m_Status;
  bool m_Registered;

  // Not allowed:
  TransactionItf();
  TransactionItf(const TransactionItf &);
  TransactionItf &operator=(const TransactionItf &);
};


}

#endif

