/*-------------------------------------------------------------------------
 *
 *   FILE
 *	robusttransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::robusttransaction class.
 *   pqxx::robusttransaction is a slower but safer transaction class
 *
 * Copyright (c) 2002-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
#include "pqxx/robusttransaction"


using namespace PGSTD;
using namespace pqxx::internal;


// TODO: Use two-phase transaction if backend supports it

#define SQL_COMMIT_WORK 	"COMMIT"
#define SQL_ROLLBACK_WORK 	"ROLLBACK"

pqxx::basic_robusttransaction::basic_robusttransaction(connection_base &C,
	const string &IsolationLevel,
	const string &TName) :
  dbtransaction(C,
      IsolationLevel,
      TName,
      "robusttransaction<"+IsolationLevel+">"),
  m_ID(oid_none),
  m_LogTable(),
  m_backendpid(-1)
{
  m_LogTable = string("PQXXLOG_") + conn().username();
}


pqxx::basic_robusttransaction::~basic_robusttransaction()
{
}


void pqxx::basic_robusttransaction::do_begin()
{
  start_backend_transaction();

  try
  {
    CreateTransactionRecord();
  }
  catch (const exception &)
  {
    // The problem here *may* be that the log table doesn't exist yet.  Create
    // one, start a new transaction, and try again.
    try { DirectExec(SQL_ROLLBACK_WORK); } catch (const exception &e) {}
    CreateLogTable();
    start_backend_transaction();
    m_backendpid = conn().backendpid();
    CreateTransactionRecord();
  }
}



void pqxx::basic_robusttransaction::do_commit()
{
  const IDType ID = m_ID;

  if (ID == oid_none)
    throw logic_error("libpqxx internal error: transaction "
		      "'" + name() + "' "
		     "has no ID");

  // Check constraints before sending the COMMIT to the database to reduce the
  // work being done inside our in-doubt window.  It also implicitly provides
  // one last check that our connection is really working before sending the
  // commit command.
  //
  // This may not be an entirely clear-cut 100% obvious unambiguous Good Thing.
  // If we lose our connection during the in-doubt window, it may be better if
  // the transaction didn't actually go though, and having constraints checked
  // first theoretically makes it more likely that it will once we get to that
  // stage.
  //
  // However, it seems reasonable to assume that most transactions will satisfy
  // their constraints.  Besides, the goal of robusttransaction is to weed out
  // indeterminacy, rather than to improve the odds of desirable behaviour when
  // all bets are off anyway.
  DirectExec("SET CONSTRAINTS ALL IMMEDIATE");

  // Here comes the critical part.  If we lose our connection here, we'll be
  // left clueless as to whether the backend got the message and is trying to
  // commit the transaction (let alone whether it will succeed if so).  This
  // case requires some special handling that makes robusttransaction what it
  // is.
  try
  {
    DirectExec(SQL_COMMIT_WORK);
  }
  catch (const exception &e)
  {
    // TODO: Can we limit this handler to broken_connection exceptions?
    m_ID = oid_none;
    if (!conn().is_open())
    {
      // We've lost the connection while committing.  We'll have to go back to
      // the backend and check our transaction log to see what happened.
      process_notice(e.what() + string("\n"));

      // See if transaction record ID exists; if yes, our transaction was
      // committed before the connection went down.  If not, the transaction
      // was aborted.
      bool Exists;
      try
      {
        Exists = CheckTransactionRecord(ID);
      }
      catch (const exception &f)
      {
	// Couldn't check for transaction record.  We're still in doubt as to
	// whether the transaction was performed.
        const string Msg = "WARNING: "
		    "Connection lost while committing transaction "
		    "'" + name() + "' (oid " + to_string(ID) + "). "
		    "Please check for this record in the "
		    "'" + m_LogTable + "' table.  "
		    "If the record exists, the transaction was executed. "
		    "If not, then it wasn't.\n";

        process_notice(Msg);
	process_notice("Could not verify existence of transaction record "
	    "because of the following error:\n");
	process_notice(string(f.what()) + "\n");

        throw in_doubt_error(Msg);
      }

      // Transaction record is gone, so all we have is a "normal" transaction
      // failure.
      if (!Exists) throw;
    }
    else
    {
      // Commit failed--probably due to a constraint violation or something
      // similar.  But we're still connected, so no worries from a consistency
      // point of view.

      // Try to delete transaction record ID, if it still exists (although it
      // really shouldn't)
      DeleteTransactionRecord(ID);
      throw;
    }
  }

  m_ID = oid_none;
  DeleteTransactionRecord(ID);
}


void pqxx::basic_robusttransaction::do_abort()
{
  m_ID = oid_none;

  // Rollback transaction.  Our transaction record will be dropped as a side
  // effect, which is what we want since "it never happened."
  DirectExec(SQL_ROLLBACK_WORK);
}


// Create transaction log table if it didn't already exist
void pqxx::basic_robusttransaction::CreateLogTable()
{
  // Create log table in case it doesn't already exist.  This code must only be
  // executed before the backend transaction has properly started.
  const string CrTab = "CREATE TABLE " + m_LogTable +
	               "("
	               "name VARCHAR(256), "
	               "date TIMESTAMP"
	               ")";

  try { DirectExec(CrTab.c_str(), 1); } catch (const exception &) { }
}


void pqxx::basic_robusttransaction::CreateTransactionRecord()
{
  // TODO: Might as well include real transaction ID and backend PID
  const string Insert = "INSERT INTO " + m_LogTable + " "
	                "(name, date) "
			"VALUES "
	                "(" +
			(name().empty() ? "null" : "'"+sqlesc(name())+"'") +
			", "
	                "CURRENT_TIMESTAMP"
	                ")";

  m_ID = DirectExec(Insert.c_str()).inserted_oid();

  if (m_ID == oid_none)
    throw runtime_error("Could not create transaction log record");
}


void pqxx::basic_robusttransaction::DeleteTransactionRecord(IDType ID) throw ()
{
  if (ID == oid_none) return;

  try
  {
    // Try very, very hard to delete record.  Specify an absurd retry count to
    // ensure that the server gets a chance to restart before we give up.
    const string Del = "DELETE FROM " + m_LogTable + " "
	               "WHERE oid=" + to_string(ID);

    DirectExec(Del.c_str(), 20);

    // Now that we've arrived here, we're almost sure that record is quite dead.
    ID = oid_none;
  }
  catch (const exception &)
  {
  }

  if (ID != oid_none) try
  {
    process_notice("WARNING: "
	           "Failed to delete obsolete transaction record with oid " +
		   to_string(ID) + " ('" + name() + "'). "
		   "Please delete it manually.  Thank you.\n");
  }
  catch (const exception &)
  {
  }
}


// Attempt to establish whether transaction record with given ID still exists
bool pqxx::basic_robusttransaction::CheckTransactionRecord(IDType ID)
{
  /* First, wait for the old backend (with the lost connection) to die.
   *
   * On 2004-09-18, Tom Lane wrote:
   *
   * I don't see any reason for guesswork.  Remember the PID of the backend
   * you were connected to.  On reconnect, look in pg_stat_activity to see
   * if that backend is still alive; if so, sleep till it's not.  Then check
   * to see if your transaction committed or not.  No need for anything so
   * dangerous as a timeout.
   */
  /* Actually, looking if the query has finished is only possible if the
   * stats_command_string has been set in postgresql.conf and we're running
   * as the postgres superuser.  If we get a string saying this option has not
   * been enabled, we must wait as if the query were still executing--and hope
   * that the backend dies.
   */
  // TODO: Tom says we need stats_start_collector, not stats_command_string!?
  /* Starting with PostgreSQL 7.4, we can use pg_locks.  The entry for the
   * zombied transaction will have a "relation" field of null, a "transaction"
   * field with the transaction ID (which we don't have--m_ID is the ID of our
   * own transaction record!), and "pid" set to our backendpid.  If the relation
   * exists but no such record is found, then the transaction is no longer
   * running.
   */
  // TODO: Support multiple means of detection for zombied transaction?
  bool hold = true;
  for (int c=20; hold && c; internal::sleep_seconds(5), --c)
  {
    const result R(DirectExec(("SELECT current_query "
	  "FROM pq_stat_activity "
	  "WHERE procpid=" + to_string(m_backendpid)).c_str()));
    hold = (!R.empty() &&
      !R[0][0].as(string()).empty() &&
      (R[0][0].as(string()) != "<IDLE>"));
  }

  if (hold)
    throw runtime_error("Old backend process stays alive too long to wait for");

  // Now look for our transaction record
  const string Find = "SELECT oid FROM " + m_LogTable + " "
	              "WHERE oid=" + to_string(ID);

  return !DirectExec(Find.c_str(), 20).empty();
}

