/*-------------------------------------------------------------------------
 *
 *   FILE
 *	robusttransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::robusttransaction class.
 *   pqxx::robusttransaction is a slower but safer transaction class
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/config.h"

#include <stdexcept>

#include "pqxx/connection_base"
#include "pqxx/result"
#include "pqxx/robusttransaction"


using namespace PGSTD;


#define SQL_BEGIN_WORK 		"BEGIN"
#define SQL_COMMIT_WORK 	"COMMIT"
#define SQL_ROLLBACK_WORK 	"ROLLBACK"

pqxx::basic_robusttransaction::basic_robusttransaction(connection_base &C, 
	const PGSTD::string &IsolationLevel,
	const string &TName) :
  dbtransaction(C, IsolationLevel, TName),
  m_ID(oid_none),
  m_LogTable()
{
  m_LogTable = string("PQXXLOG_") + conn().username();
}


pqxx::basic_robusttransaction::~basic_robusttransaction()
{
}


void pqxx::basic_robusttransaction::do_begin()
{
  // Start backend transaction
  DirectExec(SQL_BEGIN_WORK, 2, 0);

  try
  {
    CreateTransactionRecord();
  }
  catch (const exception &)
  {
    // The problem here *may* be that the log table doesn't exist yet.  Create
    // one, start a new transaction, and try again.
    DirectExec(SQL_ROLLBACK_WORK, 2, 0);
    CreateLogTable();
    DirectExec(SQL_BEGIN_WORK, 2, 0);
    CreateTransactionRecord();
  }
}



pqxx::result pqxx::basic_robusttransaction::do_exec(const char Query[])
{
  result R;
  try
  {
    R = DirectExec(Query, 0, 0);
  }
  catch (const exception &)
  {
    try { abort(); } catch (const exception &) { }
    throw;
  }

  return R;
}



void pqxx::basic_robusttransaction::do_commit()
{
  const IDType ID = m_ID;

  if (ID == oid_none) 
    throw logic_error("Internal libpqxx error: transaction " 
		      "'" + name() + "' " 
		     "has no ID");

  // Check constraints before sending the COMMIT to the database to reduce the
  // work being done inside our in-doubt window.
  //
  // This may not an entirely clear-cut 100% obvious unambiguous Good Thing.  If
  // we lose our connection during the in-doubt window, it may be better if the
  // transaction didn't actually go though, and having constraints checked first
  // theoretically makes it more likely that it will once we get to that stage.
  // However, it seems reasonable to assume that most transactions will satisfy
  // their constraints.  Besides, the goal of robusttransaction is to weed out
  // indeterminacy, rather than to improve the odds of desirable behaviour when
  // all bets are off anyway.
  DirectExec("SET CONSTRAINTS ALL IMMEDIATE", 0, 0);

  // Here comes the critical part.  If we lose our connection here, we'll be 
  // left clueless as to whether the backend got the message and is trying to
  // commit the transaction (let alone whether it will succeed if so).  This
  // case requires some special handling that makes robusttransaction what it 
  // is.
  try
  {
    DirectExec(SQL_COMMIT_WORK, 0, 0);
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
	// Couldn't reconnect to check for transaction record.  We're still in 
	// doubt as to whether the transaction was performed.
	process_notice(string(f.what()) + "\n");

        const string Msg = "WARNING: "
		    "Connection lost while committing transaction "
		    "'" + name() + "' (oid " + ToString(ID) + "). "
		    "Please check for this record in the "
		    "'" + m_LogTable + "' table.  "
		    "If the record exists, the transaction was executed. "
		    "If not, then it hasn't.";

        process_notice(Msg + "\n");
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
  DirectExec(SQL_ROLLBACK_WORK, 0, 0);
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

  try { DirectExec(CrTab.c_str(), 0, 0); } catch (const exception &) { }
}


void pqxx::basic_robusttransaction::CreateTransactionRecord()
{
  const string Insert = "INSERT INTO " + m_LogTable + " "
	                "(name, date) "
			"VALUES "
	                "(" +
	                Quote(name(), true) + ", "
	                "CURRENT_TIMESTAMP"
	                ")";

  m_ID = DirectExec(Insert.c_str(), 0, 0).inserted_oid();

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
	               "WHERE oid=" + ToString(ID);

    DirectExec(Del.c_str(), 20, 0);

    // Now that we've arrived here, we're almost sure that record is quite dead.
    ID = oid_none;
  }
  catch (const exception &e)
  {
  }

  if (ID != oid_none) try
  {
    process_notice("WARNING: "
	           "Failed to delete obsolete transaction record with oid " + 
		   ToString(ID) + " ('" + name() + "'). "
		   "Please delete it manually.  Thank you.\n");
  }
  catch (const exception &)
  {
  }
}


// Attempt to establish whether transaction record with given ID still exists
bool pqxx::basic_robusttransaction::CheckTransactionRecord(IDType ID)
{
  const string Find = "SELECT oid FROM " + m_LogTable + " "
	              "WHERE oid=" + ToString(ID);

  return !DirectExec(Find.c_str(), 20, 0).empty();
}

