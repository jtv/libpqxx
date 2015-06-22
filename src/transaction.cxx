/*-------------------------------------------------------------------------
 *
 *   FILE
 *	transaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::transaction class.
 *   pqxx::transaction represents a regular database transaction
 *
 * Copyright (c) 2001-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <stdexcept>

#include "pqxx/connection_base"
#include "pqxx/result"
#include "pqxx/transaction"


pqxx::basic_transaction::basic_transaction(
	connection_base &C,
	const std::string &IsolationLevel,
	readwrite_policy rw) :
  namedclass("transaction"),
  dbtransaction(C, IsolationLevel, rw)
{
}


void pqxx::basic_transaction::do_commit()
{
  try
  {
    DirectExec(internal::sql_commit_work);
  }
  catch (const std::exception &e)
  {
    if (!conn().is_open())
    {
      // We've lost the connection while committing.  There is just no way of
      // telling what happened on the other end.  >8-O
      process_notice(e.what() + std::string("\n"));

      const std::string Msg = "WARNING: "
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

