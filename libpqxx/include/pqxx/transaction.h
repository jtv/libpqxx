/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transaction.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::Transaction class.
 *   pqxx::Transaction represents a database transaction
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TRANSACTION_H
#define PQXX_TRANSACTION_H

/* While you may choose to create your own Transaction object to interface to 
 * the database backend, it is recommended that you wrap your transaction code 
 * into a Transactor code instead and let the Transaction be created for you.
 * See pqxx/transactor.h for more about Transactor.
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

#include "pqxx/connection.h"
#include "pqxx/transactionitf.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


// TODO: Any user-friendliness we can add to invoking stored procedures?

namespace pqxx
{


/** Use a Transaction object to enclose operations on a database in a single
 * "unit of work."  This ensures that the whole series of operations either
 * succeeds as a whole or fails completely.  In no case will it leave half
 * finished work behind in the database.
 *
 * Queries can currently only be issued through a Transaction.
 *
 * Once processing on a transaction has succeeded and any changes should be
 * allowed to become permanent in the database, call Commit().  If something
 * has gone wrong and the changes should be forgotten, call Abort() instead.
 * If you do neither, an implicit Abort() is executed at destruction time.
 *
 * It is an error to abort a Transaction that has already been committed, or to 
 * commit a transaction that has already been aborted.  Aborting an already 
 * aborted transaction or committing an already committed one has been allowed 
 * to make errors easier to deal with.  Repeated aborts or commits have no
 * effect after the first one.
 *
 * Database transactions are not suitable for guarding long-running processes.
 * If your transaction code becomes too long or too complex, please consider
 * ways to break it up into smaller ones.  There's no easy, general way to do
 * this since application-specific considerations become important at this 
 * point.
 */
class PQXX_LIBEXPORT Transaction : public TransactionItf
{
public:
  /// Create a transaction.  The optional name, if given, must begin with a
  /// letter and may contain letters and digits only.
  explicit Transaction(Connection &, 
		       PGSTD::string Name=PGSTD::string());		//[t1]

  virtual ~Transaction();						//[t1]

private:
  virtual void DoBegin();						//[t1]
  virtual Result DoExec(const char[]);					//[t1]
  virtual void DoCommit();						//[t1]
  virtual void DoAbort();						//[t13]
};

}

#endif

