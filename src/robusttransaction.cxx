/*-------------------------------------------------------------------------
 *
 *   FILE
 *	robusttransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::robusttransaction class.
 *   pqxx::robusttransaction is a slower but safer transaction class
 *
 * Copyright (c) 2002-2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
#include "pqxx/robusttransaction"


using namespace PGSTD;
using namespace pqxx::internal;


// TODO: Log username in more places.

pqxx::basic_robusttransaction::basic_robusttransaction(
	connection_base &C,
	const PGSTD::string &IsolationLevel,
	const PGSTD::string &table_name) :
  namedclass("robusttransaction"),
  dbtransaction(C, IsolationLevel),
  m_record_id(0),
  m_xid(),
  m_LogTable(table_name),
  m_sequence(),
  m_backendpid(-1)
{
  if (table_name.empty()) m_LogTable = "pqxx_robusttransaction_log";
  m_sequence = m_LogTable + "_seq";
}


pqxx::basic_robusttransaction::~basic_robusttransaction()
{
}


void pqxx::basic_robusttransaction::do_begin()
{
  try
  {
    CreateTransactionRecord();
  }
  catch (const exception &)
  {
    // The problem here *may* be that the log table doesn't exist yet.  Create
    // one, start a new transaction, and try again.
    try { dbtransaction::do_abort(); } catch (const exception &) {}
    CreateLogTable();
    dbtransaction::do_begin();
    m_backendpid = conn().backendpid();
    CreateTransactionRecord();
  }

  dbtransaction::do_begin();

  // If this transaction commits, the transaction record should also be gone.
  DirectExec(sql_delete().c_str());

  if (conn().server_version() >= 80300)
    DirectExec("SELECT txid_current()")[0][0].to(m_xid);
}



void pqxx::basic_robusttransaction::do_commit()
{
  if (!m_record_id)
    throw internal_error("transaction '" + name() + "' has no ID");

  // Check constraints before sending the COMMIT to the database to reduce the
  // work being done inside our in-doubt window.
  try
  {
    DirectExec("SET CONSTRAINTS ALL IMMEDIATE");
  }
  catch (...)
  {
    do_abort();
    throw;
  }

  // Here comes the critical part.  If we lose our connection here, we'll be
  // left clueless as to whether the backend got the message and is trying to
  // commit the transaction (let alone whether it will succeed if so).  That
  // case requires some special handling that makes robusttransaction what it
  // is.
  try
  {
    DirectExec(sql_commit_work);

    // If we make it here, great.  Normal, successful commit.
    m_record_id = 0;
    return;
  }
  catch (const broken_connection &)
  {
    // Oops, lost connection at the crucial moment.  Fall through to in-doubt
    // handling below.
  }
  catch (...)
  {
    if (conn().is_open())
    {
      // Commit failed--probably due to a constraint violation or something
      // similar.  But we're still connected, so no worries from a consistency
      // point of view.
      do_abort();
      throw;
    }
    // Otherwise, fall through to in-doubt handling.
  }

  // If we get here, we're in doubt.  Talk to the backend, figure out what
  // happened.  If the transaction record still exists, the transaction failed.
  // If not, it succeeded.

  bool exists;
  try
  {
    exists = CheckTransactionRecord();
  }
  catch (const exception &f)
  {
    // Couldn't check for transaction record.  We're still in doubt as to
    // whether the transaction was performed.
    const string Msg = "WARNING: "
	"Connection lost while committing transaction "
	"'" + name() + "' (id " + to_string(m_record_id) + ", "
	"transaction_id " + m_xid + "). "
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

  // Transaction record is still there, so the transaction failed and all we
  // have is a "normal" transaction failure.
  if (exists)
  {
    do_abort();
    throw broken_connection("Connection lost while committing.");
  }

  // Otherwise, the transaction succeeded.  Forget there was ever an error.
}


void pqxx::basic_robusttransaction::do_abort()
{
  dbtransaction::do_abort();
  DeleteTransactionRecord();
}


// Create transaction log table if it didn't already exist
void pqxx::basic_robusttransaction::CreateLogTable()
{
  // Create log table in case it doesn't already exist.  This code must only be
  // executed before the backend transaction has properly started.
  string CrTab = "CREATE TABLE \"" + m_LogTable + "\" ("
	"id INTEGER NOT NULL, "
        "username VARCHAR(256), "
	"transaction_id xid, "
	"name VARCHAR(256), "
	"date TIMESTAMP NOT NULL"
	")";

  try
  {
    DirectExec(CrTab.c_str(), 1);
  }
  catch (const exception &e)
  {
    conn().process_notice(
	"Could not create transaction log table: " + string(e.what()));
  }

  try
  {
    DirectExec(("CREATE SEQUENCE " + m_sequence).c_str());
  }
  catch (const exception &e)
  {
    conn().process_notice(
	"Could not create transaction log sequence: " + string(e.what()));
  }
}


void pqxx::basic_robusttransaction::CreateTransactionRecord()
{
  static const string Fail = "Could not create transaction log record: ";

  // Clean up old transaction records.
  DirectExec((
	"DELETE FROM " + m_LogTable + " "
	"WHERE date < CURRENT_TIMESTAMP - '30 days'::interval").c_str());

  // Allocate id.
  const string sql_get_id("SELECT nextval(" + quote(m_sequence) + ")");
  DirectExec(sql_get_id.c_str())[0][0].to(m_record_id);

  DirectExec((
	"INSERT INTO \"" + m_LogTable + "\" "
	"(id, username, name, date) "
	"VALUES "
	"(" +
	to_string(m_record_id) + ", " +
        quote(conn().username()) + ", " +
	(name().empty() ? "NULL" : quote(name())) + ", "
	"CURRENT_TIMESTAMP"
	")").c_str());
}


string pqxx::basic_robusttransaction::sql_delete() const
{
  return "DELETE FROM \"" + m_LogTable + "\" "
	"WHERE id = " + to_string(m_record_id);
}


void pqxx::basic_robusttransaction::DeleteTransactionRecord() throw ()
{
  if (!m_record_id) return;

  try
  {
    const string Del = sql_delete();

    reactivation_avoidance_exemption E(conn());
    DirectExec(Del.c_str(), 20);

    // Now that we've arrived here, we're about as sure as we can be that that
    // record is quite dead.
    m_record_id = 0;
  }
  catch (const exception &)
  {
  }

  if (m_record_id != 0) try
  {
    process_notice("WARNING: "
	           "Failed to delete obsolete transaction record with id " +
		   to_string(m_record_id) + " ('" + name() + "'). "
		   "Please delete it manually.  Thank you.\n");
  }
  catch (const exception &)
  {
  }
}


// Attempt to establish whether transaction record with given ID still exists
bool pqxx::basic_robusttransaction::CheckTransactionRecord()
{
  bool hold = true;
  for (int c=20; hold && c; internal::sleep_seconds(5), --c)
  {
    if (conn().server_version() > 80300)
    {
      const string query(
	"SELECT " + m_xid + " >= txid_snapshot_xmin(txid_current_snapshot())");
      DirectExec(query.c_str())[0][0].to(hold);
    }
    else
    {
      /* Wait for the old backend (with the lost connection) to die.
       *
       * Actually this is only possible if stats_command_string (or maybe
       * stats_start_collector?) has been set in postgresql.conf and we're
       * running as the postgres superuser.
       *
       * Starting with 7.4, we could also use pg_locks.  The entry for a zombied
       * transaction will have a "relation" field of null, a "transaction" field
       * with the transaction ID, and "pid" set to our backend pid.  If the
       * relation exists but no such record is found, then the transaction is no
       * longer running.
       */
      const result R(DirectExec((
	"SELECT current_query "
	"FROM pq_stat_activity "
	"WHERE procpid = " + to_string(m_backendpid)).c_str()));
      hold = !R.empty();
    }
  }

  if (hold)
    throw in_doubt_error(
	"Old backend process stays alive too long to wait for.");

  // Now look for our transaction record
  const string Find = "SELECT id FROM \"" + m_LogTable + "\" "
	              "WHERE "
                        "id = " + to_string(m_record_id) + " AND "
                        "user = " + conn().username();

  return !DirectExec(Find.c_str(), 20).empty();
}

