/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_nontransaction.h
 *
 *   DESCRIPTION
 *      definition of the Pg::NonTransaction class.
 *   Pg::NonTransaction provides nontransactional database access
 *
 * Copyright (c) 2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_NONTRANSACTION_H
#define PG_NONTRANSACTION_H

#include "pg_connection.h"
#include "pg_result.h"
#include "pg_transactionitf.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace Pg
{

class NonTransaction : public TransactionItf
{
public:
  // Create a transaction.  The optional name, if given, must begin with a
  // letter and may contain letters and digits only.
  explicit NonTransaction(Connection &C, 
		          PGSTD::string NName=PGSTD::string()) :
    TransactionItf(C, NName) { Begin(); }

  virtual ~NonTransaction();

private:
  virtual void DoBegin() {}
  virtual Result DoExec(const char C[]);
  virtual void DoCommit() {}
  virtual void DoAbort() {}
};

}

#endif

