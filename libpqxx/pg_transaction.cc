/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_transaction.cc
 *
 *   DESCRIPTION
 *      implementation of the Pg::Transaction class.
 *   Pg::Transaction represents a database transaction
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pg_connection.h"
#include "pg_result.h"
#include "pg_tablestream.h"
#include "pg_transaction.h"


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


Pg::Transaction::Transaction(Connection &C, string TName) :
  TransactionItf(C, TName)
{
  Begin();
}



Pg::Transaction::~Transaction()
{
  End();
}



void Pg::Transaction::DoBegin()
{
  // Start backend transaction
  DirectExec(SQL_BEGIN_WORK, 2, 0);
}



Pg::Result Pg::Transaction::DoExec(const char C[])
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



void Pg::Transaction::DoCommit()
{
  try
  {
    DirectExec(SQL_COMMIT_WORK, 0, 0);
  }
  catch (const exception &e)
  {
    if (!Conn().IsOpen())
    {
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
      throw;
    }
  }
}


void Pg::Transaction::DoAbort()
{
  DirectExec(SQL_ROLLBACK_WORK, 0, 0);
}


