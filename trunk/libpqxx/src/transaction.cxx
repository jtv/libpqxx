/*-------------------------------------------------------------------------
 *
 *   FILE
 *	transaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::transaction class.
 *   pqxx::transaction represents a regular database transaction
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <stdexcept>

#include "pqxx/connection_base"
#include "pqxx/result"
#include "pqxx/transaction"


using namespace PGSTD;


#define SQL_COMMIT_WORK 	"COMMIT"
#define SQL_ROLLBACK_WORK 	"ROLLBACK"


pqxx::basic_transaction::basic_transaction(connection_base &C,
    const string &IsolationLevel,
    const string &TName) :
  dbtransaction(C, IsolationLevel, TName, "transaction<"+IsolationLevel+">")
{
}


void pqxx::basic_transaction::do_begin()
{
  start_backend_transaction();
}


void pqxx::basic_transaction::do_commit()
{
  try
  {
    DirectExec(SQL_COMMIT_WORK);
  }
  catch (const exception &e)
  {
    if (!conn().is_open())
    {
      // We've lost the connection while committing.  There is just no way of
      // telling what happened on the other end.  >8-O
      process_notice(e.what() + string("\n"));

      const string Msg = "WARNING: "
		  "Connection lost while committing transaction "
		  "'" + name() + "'. "
		  "There is no way to tell whether the transaction succeeded "
		  "or was aborted except to check manually.";

      process_notice(Msg + "\n");
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


void pqxx::basic_transaction::do_abort()
{
  DirectExec(SQL_ROLLBACK_WORK);
  reactivation_avoidance_clear();
}


