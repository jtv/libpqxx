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
 *
 * When a user first uses a RobustTransaction in a database, the class will
 * attempt to create a log table there to keep vital transaction-related state
 * information in.  This table, located in that same database, will be called
 * PQXXLOG_*user*, where *user* is the PostgreSQL username for that user.  If
 * the log table can not be created, the transaction fails immediately.
 *
 * If the user does not have permission to create the log table, the database
 * administrator may create one for him beforehand, and give ownership (or at
 * least full insert/update rights) to the user.  The table must contain two
 * non-unique fields (which will never be null): "name" (of text type, 
 * varchar(256) by default) and "date" (of timestamp type).  Older verions of
 * RobustTransaction also added a unique "id" field; this field is now obsolete
 * and the log table's implicit oids are used instead.  The log tables' names
 * may be made configurable in a future version of libpqxx.
 *
 * The transaction log table contains records describing unfinished 
 * transactions, i.e. ones that have been started but not, as far as the client
 * knows, committed or aborted.  This can mean any of the following:
 *
 * 1. The transaction is in progress.  Since backend transactions can't run for
 * extended periods of time, this can only be the case if the log record's
 * timestamp (compared to the server's clock) is not very old, provided of 
 * course that the server's system clock hasn't just made a radical jump.
 *
 * 2. The client's connection to the server was lost, just when the client was
 * committing the transaction, and the client so far has not been able to
 * re-establish the connection to verify whether the transaction was actually
 * completed or rolled back by the server.  This is a serious (and luckily a
 * rare) condition and requires manual inspection of the database to determine
 * what happened.  RobustTransaction will emit clear and specific warnings to
 * this effect, and will identify the log record describing the transaction in
 * question.
 *
 * 3. The transaction was completed (either by commit or by rollback), but the
 * client's connection was durably lost just as it tried to clean up the log
 * record.  Again, RobustTransaction will emit a clear and specific warning to
 * tell you about this and request that the record be deleted as soon as 
 * possible.
 *
 * 4. The client has gone offline at any time while in one of the preceding 
 * states.  This also requires manual intervention, but the client obviously is
 * not able to issue a warning.
 *
 * It is safe to drop a log table when it is not in use (ie., it is empty or all
 * records in it represent states 2-4 above).  RobustTransaction will attempt to
 * recreate the table at its next time of use.
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
};


}


#endif

