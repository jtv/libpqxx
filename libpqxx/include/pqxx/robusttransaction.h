/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/robusttransaction.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::RobustTransaction class.
 *   pqxx::RobustTransaction is a slower but safer transaction class
 *
 * Copyright (c) 2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_ROBUSTTRANSACTION_H
#define PQXX_ROBUSTTRANSACTION_H


#include "pqxx/connection.h"
#include "pqxx/transactionitf.h"


/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{

/// Slower, better fortified version of Transaction.
/** RobustTransaction is similar to Transaction, but spends more effort (and
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
 *
 * Besides being slower, you may find that RobustTransaction actually fails
 * more instead of less often than a normal Transaction.  This is due to the
 * added work and complexity.  What RobustTransaction tries to achieve is to
 * be more deterministic, not more successful per se.
 */
class PQXX_LIBEXPORT RobustTransaction : public TransactionItf
{
public:
  explicit RobustTransaction(Connection &, 
		             PGSTD::string Name=PGSTD::string());	//[t16]

  virtual ~RobustTransaction();						//[t16]

private:
  typedef unsigned long IDType;
  IDType m_ID;
  PGSTD::string m_LogTable;

  virtual void DoBegin();						//[t18]
  virtual Result DoExec(const char[]);					//[t18]
  virtual void DoCommit();						//[t16]
  virtual void DoAbort();						//[t18]

  void CreateLogTable();
  void CreateTransactionRecord();
  void DeleteTransactionRecord(IDType ID) throw ();
  bool CheckTransactionRecord(IDType ID);

  static PGSTD::string s_SeqPostfix;
  static PGSTD::string s_IdxPostfix;
};


}


#endif

