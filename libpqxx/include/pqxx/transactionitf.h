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
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_TRANSACTIONITF_H
#define PG_TRANSACTIONITF_H


/* End-user programs need not include this file, unless they define their own
 * transaction classes.  This is not something the typical program should want
 * to do.
 *
 * However, reading this file is worthwhile because it defines the public
 * interface for the available transaction classes such as Transaction and 
 * NonTransaction.
 */


#include "pqxx/connection.h"
#include "pqxx/util.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


// TODO: Any user-friendliness we can add to invoking stored procedures?

namespace pqxx
{
class Connection; 	// See pqxx/connection.h
class Result; 		// See pqxx/result.h
class TableStream;	// See pqxx/tablestream.h


template<> inline PGSTD::string Classname(const TableStream *) 
{ 
  return "TableStream"; 
}



// Interface definition (and common code) for "transaction" classes.  All 
// database access must be channeled through one of these classes for safety,
// although not all implementations of this interface need to provide full
// transactional integrity.
class PQXX_LIBEXPORT TransactionItf
{
public:
  virtual ~TransactionItf() =0;						//[t1]

  void Commit();							//[t1]
  void Abort();								//[t10]

  // Execute query directly
  Result Exec(const char[]);						//[t1]

  void ProcessNotice(const char Msg[]) { m_Conn.ProcessNotice(Msg); }	//[t1]
  void ProcessNotice(PGSTD::string Msg) { m_Conn.ProcessNotice(Msg); }	//[t1]

  PGSTD::string Name() const { return m_Name; }				//[t1]

protected:
  // Create a transaction.  The optional name, if given, must begin with a
  // letter and may contain letters and digits only.
  explicit TransactionItf(Connection &, 
		          PGSTD::string Name=PGSTD::string());

  // Begin transaction.  To be called by implementing class, typically from 
  // constructor.
  void Begin();

  // End transaction.  To be called by implementing class' destructor 
  void End() throw ();

  // To be implemented by implementing class.
  virtual void DoBegin() =0;
  virtual Result DoExec(const char C[]) =0;
  virtual void DoCommit() =0;
  virtual void DoAbort() =0;

  // For use by implementing class:

  // Execute query on connection directly
  Result DirectExec(const char C[], int Retries, const char OnReconnect[]);
  Connection &Conn() const { return m_Conn; }

 
private:
  /* A Transaction goes through the following stages in its lifecycle:
   *  - nascent: the transaction hasn't actually begun yet.  If our connection 
   *    fails at this stage, the Connection may recover and the Transaction can 
   *    attempt to establish itself again.
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
  void BeginCopyRead(PGSTD::string Table) { m_Conn.BeginCopyRead(Table); }
  bool ReadCopyLine(PGSTD::string &L) { return m_Conn.ReadCopyLine(L); }
  friend class TableWriter;
  void BeginCopyWrite(PGSTD::string Table) {m_Conn.BeginCopyWrite(Table);}
  void WriteCopyLine(PGSTD::string L) { m_Conn.WriteCopyLine(L); }

  Connection &m_Conn;

  Status m_Status;
  PGSTD::string m_Name;
  int m_UniqueCursorNum;
  Unique<TableStream> m_Stream;
  bool m_Registered;

  // Not allowed:
  TransactionItf();
  TransactionItf(const TransactionItf &);
  TransactionItf &operator=(const TransactionItf &);
};


// An exception that might be thrown in rare cases where the connection to the
// database is lost while finishing a database transaction, and there's no way
// of telling whether it was actually executed by the backend.  In this case
// the database is left in an indeterminate (but consistent) state, and only
// manual inspection will tell which is the case.
// TODO: Get guarantee that libpq will not let this happen, or deal with it
class PQXX_LIBEXPORT in_doubt_error : public PGSTD::runtime_error
{
public:
  explicit in_doubt_error(const PGSTD::string &whatarg) : 
    PGSTD::runtime_error(whatarg) {}
};

}

#endif

