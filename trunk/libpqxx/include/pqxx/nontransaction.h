/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/nontransaction.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::NonTransaction class.
 *   pqxx::NonTransaction provides nontransactional database access
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_NONTRANSACTION_H
#define PQXX_NONTRANSACTION_H

#include "pqxx/connectionitf.h"
#include "pqxx/result.h"
#include "pqxx/transactionitf.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{
/// Simple "Transaction" class offering no transactional integrity.
/**
 * NonTransaction, like Transaction or any other TransactionItf-derived class,
 * provides access to a database through a connection.  Unlike its siblings,
 * however, NonTransaction does not maintain any kind of transactional 
 * integrity.  This may be useful eg. for read-only access to the database that
 * does not require a consistent, atomic view on its data.
 * As a side effect, you can keep a NonTransaction open for as long as you like.
 * Actual back-end transactions are limited in lifespan, and will sometimes fail
 * just because they took to long to execute.
 *
 * Some database features that require a backend transaction, such as cursors or
 * large objects, will not work in a NonTransaction.
 */
class PQXX_LIBEXPORT NonTransaction : public TransactionItf
{
public:
  /// Constructor.
  /** Create a "dummy" transaction.
   * @param C the connection that this "transaction" will operate on.
   * @param NName an optional name for the transaction, beginning with a letter
   * and containing only letters and digits.
   */
  explicit NonTransaction(ConnectionItf &C, 
		          const PGSTD::string &NName=PGSTD::string()) :	//[t14]
    TransactionItf(C, NName) { Begin(); }

  virtual ~NonTransaction();						//[t14]

private:
  virtual void DoBegin() {}						//[t14]
  virtual Result DoExec(const char C[]);				//[t14]
  virtual void DoCommit() {}						//[t14]
  virtual void DoAbort() {}						//[t20]
};

}

#endif

