/*-------------------------------------------------------------------------
 *
 *   FILE
 *	transaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::Transaction class.
 *   pqxx::Transaction represents a database transaction
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pqxx/connection.h"
#include "pqxx/result.h"
#include "pqxx/transaction.h"


using namespace PGSTD;


#define SQL_BEGIN_WORK 		"BEGIN"
#define SQL_COMMIT_WORK 	"COMMIT"
#define SQL_ROLLBACK_WORK 	"ROLLBACK"


pqxx::Transaction::Transaction(Connection &C, string TName) :
  TransactionItf(C, TName)
{
  Begin();
}



pqxx::Transaction::~Transaction()
{
  End();
}



void pqxx::Transaction::DoBegin()
{
  // Start backend transaction
  DirectExec(SQL_BEGIN_WORK, 2, 0);
}



pqxx::Result pqxx::Transaction::DoExec(const char C[])
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



void pqxx::Transaction::DoCommit()
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


void pqxx::Transaction::DoAbort()
{
  DirectExec(SQL_ROLLBACK_WORK, 0, 0);
}


