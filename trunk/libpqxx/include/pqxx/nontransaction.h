/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/nontransaction.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::NonTransaction class.
 *   pqxx::NonTransaction provides nontransactional database access
 *
 * Copyright (c) 2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_NONTRANSACTION_H
#define PG_NONTRANSACTION_H

#include "pqxx/connection.h"
#include "pqxx/result.h"
#include "pqxx/transactionitf.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{

class PQXX_LIBEXPORT NonTransaction : public TransactionItf
{
public:
  // Create a transaction.  The optional name, if given, must begin with a
  // letter and may contain letters and digits only.
  explicit NonTransaction(Connection &C, 
		          PGSTD::string NName=PGSTD::string()) :	//[t14]
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

