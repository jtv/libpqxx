/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_robusttransaction.cc
 *
 *   DESCRIPTION
 *      implementation of the Pg::RobustTransaction class.
 *   Pg::RobustTransaction is a slower but safer transaction class
 *
 * Copyright (c) 2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pg_connection.h"
#include "pg_result.h"
#include "pg_robusttransaction.h"


using namespace PGSTD;


#ifdef DIALECT_POSTGRESQL
#define SQL_BEGIN_WORK 		"BEGIN"
#define SQL_COMMIT_WORK 	"COMMIT"
#define SQL_ROLLBACK_WORK 	"ROLLBACK"
#else
#define SQL_BEGIN_WORK 		"BEGIN WORK"
#define SQL_COMMIT_WORK 	"COMMIT WORK"
#define SQL_ROLLBACK_WORK 	"ROLLBACK WORK"
#endif // DIALECT_POSTGRESQL 


Pg::RobustTransaction::RobustTransaction(Connection &C, string TName) :
  TransactionItf(C, TName)
{
  Begin();
}



Pg::RobustTransaction::~RobustTransaction()
{
  End();
}



void Pg::RobustTransaction::DoBegin()
{
  // TODO: Create user-specific "transaction log" table, if necessary
  // TODO: Delete information on previous transaction, if any
 
  // Start backend transaction
  DirectExec(SQL_BEGIN_WORK, 2, 0);

  // TODO: Create new transaction record
}



Pg::Result Pg::RobustTransaction::DoExec(const char C[])
{
  Result R;
  try
  {
    R = DirectExec(C, 0, SQL_BEGIN_WORK);
  }
  catch (const exception &)
  {
    try
    {
      Abort();
    }
    catch (const exception &)
    {
    }
    throw;
  }

  return R;
}



void Pg::RobustTransaction::DoCommit()
{
  try
  {
    DirectExec(SQL_COMMIT_WORK, 0, 0);
    // TODO: Delete transaction record
  }
  catch (const exception &e)
  {
    if (!Conn().IsOpen())
    {
      // TODO: Attempt to reconnect & check transaction record; throw if fails

      // TODO: Delete transaction record
 
      // We've lost the connection while committing.  There is just no way of
      // telling what happened on the other end.  >8-O
      ProcessNotice(e.what() + string("\n"));

      const string Msg = "WARNING: "
		  "Connection lost while committing transaction "
		  "'" + Name() + "'. "
		  "There is no way to tell whether the transaction succeeded "
		  "or was aborted except to check manually.";

      ProcessNotice(Msg + "\n");
      throw in_doubt_error(Msg);
    }
    else
    {
      // Commit failed--probably due to a constraint violation or something
      // similar.
      // TODO: Delete transaction record
      throw;
    }
  }
}


void Pg::RobustTransaction::DoAbort()
{
  // Rollback transaction.  Our transaction record will be dropped as a side
  // effect, which is what we want since "it never happened."
  DirectExec(SQL_ROLLBACK_WORK, 0, 0);
}


