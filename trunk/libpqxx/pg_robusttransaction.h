/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_robusttransaction.h
 *
 *   DESCRIPTION
 *      definition of the Pg::RobustTransaction class.
 *   Pg::RobustTransaction is a slower but safer transaction class
 *
 * Copyright (c) 2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_ROBUSTTRANSACTION_H
#define PG_ROBUSTTRANSACTION_H

/* RobustTransaction is similar to Transaction, but spends more effort (and
 * performance!) to deal with the hopefully rare case that the connection to
 * the backend is lost just as the current transaction is being committed.  In
 * this case, there is no way to determine whether the backend managed to
 * commit the transaction before noticing the loss of connection.
 * 
 * In such cases, transaction class tries to reconnect to the database and
 * figure out what happened.  It will need to store and manage some information
 * (pretty much a user-level transaction log) in the back-end for each and
 * every transaction just on the off chance that this problem might occur.
 * This service level was made optional since you may not want to pay this
 * overhead where it is not necessary.  Certainly the use of this class makes
 * no sense for local connections, or for transactions that read the database
 * but never modify it, or for noncritical database manipulations.
 */
#include "pg_connection.h"
#include "pg_transactionitf.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace Pg
{


class RobustTransaction : public TransactionItf
{
public:
  explicit RobustTransaction(Connection &, 
		             PGSTD::string Name=PGSTD::string());

  virtual ~RobustTransaction();

private:
  virtual void DoBegin();
  virtual Result DoExec(const char[]);
  virtual void DoCommit();
  virtual void DoAbort();
};

}

#endif

