/*-------------------------------------------------------------------------
 *
 *   FILE
 *	transaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::transaction class.
 *   pqxx::transaction represents a regular database transaction
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pqxx/connection_base.h"
#include "pqxx/result.h"
#include "pqxx/transaction.h"


using namespace PGSTD;


#define SQL_COMMIT_WORK 	"COMMIT"
#define SQL_ROLLBACK_WORK 	"ROLLBACK"


void pqxx::basic_transaction::DoBegin()
{
  // Start backend transaction
  DirectExec(StartCmd().c_str(), 2, 0);
}



pqxx::result pqxx::basic_transaction::DoExec(const char Query[])
{
  result R;
  try
  {
    R = DirectExec(Query, 0, StartCmd().c_str());
  }
  catch (const exception &)
  {
    try { Abort(); } catch (const exception &) { }
    throw;
  }

  return R;
}



void pqxx::basic_transaction::DoCommit()
{
  try
  {
    DirectExec(SQL_COMMIT_WORK, 0, 0);
  }
  catch (const exception &e)
  {
    if (!Conn().is_open())
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


void pqxx::basic_transaction::DoAbort()
{
  DirectExec(SQL_ROLLBACK_WORK, 0, 0);
}


