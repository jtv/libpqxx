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
  TransactionItf(C, TName),
  m_ID(0)
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
  // TODO: SOMEHOW recognize & garbage-collect stale transactions belonging to us?
  // TODO: Delete information on previous transaction, if any
 
  // Start backend transaction
  DirectExec(SQL_BEGIN_WORK, 2, 0);

  // TODO: Create new transaction record (repeat while ID==0, zero has special meaning to us)
  // TODO: Set m_ID to object ID of new transaction record
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
    // TODO: Delete transaction record m_ID
    m_ID = 0;
  }
  catch (const exception &e)
  {
    const unsigned long ID = m_ID;
    m_ID = 0;

    if (!Conn().IsOpen())
    {
      // We've lost the connection while committing.  There is just no way of
      // telling what happened on the other end.  >8-O
      ProcessNotice(e.what() + string("\n"));

      // TODO: Attempt to reconnect; throw in_doubt_error if fails consistently (try waiting a while)
 
      // TODO: See if transaction record ID exists
      // TODO: 	If no: 	throw
      // TODO:  If yes:	delete transaction record ID
 
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
      // TODO: Delete transaction record ID, if it still exists
      throw;
    }
  }
}


void Pg::RobustTransaction::DoAbort()
{
  m_ID = 0;

  // Rollback transaction.  Our transaction record will be dropped as a side
  // effect, which is what we want since "it never happened."
  DirectExec(SQL_ROLLBACK_WORK, 0, 0);
}


