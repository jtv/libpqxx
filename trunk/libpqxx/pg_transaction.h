/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_transaction.h
 *
 *   DESCRIPTION
 *      definition of the Pg::Transaction class.
 *   Pg::Transaction represents a database transaction
 *
 * Copyright (c) 2001, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_TRANSACTION_H
#define PG_TRANSACTION_H

/* While you may choose to create your own Transaction object to interface to 
 * the database backend, it is recommended that you wrap your transaction code 
 * into a Transactor code instead and let the Transaction be created for you.
 * See pg_transactor.h for more about Transactor.
 *
 * If you should find that using a Transactor makes your code less portable or 
 * too complex, go ahead, create your own Transaction anyway.
 */

// Usage example: double all wages
//
// extern Connection C;
// Transaction T(C);
// try
// {
//   T.Exec("UPDATE employees SET wage=wage*2");
//   T.Commit();	// NOTE: do this inside catch block
// } 
// catch (const exception &e)
// {
//   cerr << e.what() << endl;
//   T.Abort();		// Usually not needed; same happens when T's life ends.
// }

#include "pg_connection.h"

// TODO: Any user-friendliness we can add to invoking stored procedures?

namespace Pg
{
class Connection; 	// See pg_connection.h
class Result; 		// See pg_result.h
class TableStream;	// See pg_tablestream.h

template<> inline PGSTD::string Classname(const TableStream *)
{
  return "Stream";
}


// Use a Transaction object to enclose operations on a database in a single
// "unit of work."  This ensures that the whole series of operations either
// succeeds as a whole or fails completely.  In no case will it leave half
// finished work behind in the database.
//
// Queries can currently only be issued through a Transaction.
//
// Once processing on a transaction has succeeded and any changes should be
// allowed to become permanent in the database, call Commit().  If something
// has gone wrong and the changes should be forgotten, call Abort() instead.
// If you do neither, an implicit Abort() is executed at destruction time.
//
// It is an error to abort a Transaction that has already been committed, or to 
// commit a transaction that has already been aborted.  Aborting an already 
// aborted transaction or committing an already committed one has been allowed 
// to make errors easier to deal with.  Repeated aborts or commits have no
// effect after the first one.
//
// Database transactions are not suitable for guarding long-running processes.
// If your transaction code becomes too long or too complex, please consider
// ways to break it up into smaller ones.  There's no easy, general way to do
// this since application-specific considerations become important at this 
// point.
class Transaction
{
public:
  // Create a transaction.  The optional name, if given, must begin with a
  // letter and may contain letters and digits only.
  explicit Transaction(Connection &, PGSTD::string Name=PGSTD::string());
  ~Transaction();

  void Commit();
  void Abort();

  // Execute query directly
  Result Exec(const char[]);
  void ProcessNotice(const char Msg[]) { m_Conn.ProcessNotice(Msg); }
  void ProcessNotice(PGSTD::string Msg) { m_Conn.ProcessNotice(Msg); }

  PGSTD::string Name() const { return m_Name; }

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
   */
  enum Status { st_nascent, st_active, st_aborted, st_committed };

  void Begin();

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
  void BeginCopyWrite(PGSTD::string Table) {return m_Conn.BeginCopyWrite(Table);}
  void WriteCopyLine(PGSTD::string L) { m_Conn.WriteCopyLine(L); }

  Connection &m_Conn;

  Status m_Status;
  PGSTD::string m_Name;
  int m_UniqueCursorNum;
  Unique<TableStream> m_Stream;

  // Not allowed:
  Transaction();
  Transaction(const Transaction &);
  Transaction &operator=(const Transaction &);
};

}

#endif

