/*-------------------------------------------------------------------------
 *
 *   FILE
 *	robusttransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::RobustTransaction class.
 *   pqxx::RobustTransaction is a slower but safer transaction class
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pqxx/connection_base.h"
#include "pqxx/result.h"
#include "pqxx/robusttransaction.h"


using namespace PGSTD;


#define SQL_BEGIN_WORK 		"BEGIN"
#define SQL_COMMIT_WORK 	"COMMIT"
#define SQL_ROLLBACK_WORK 	"ROLLBACK"

// Workaround: define alias for InvalidOid to reduce g++ warnings about
// C-style cast used in InvalidOid's definition
namespace
{
const Oid pqxxInvalidOid(InvalidOid);
} // namespace

pqxx::RobustTransaction::RobustTransaction(Connection_base &C, 
                                           const string &TName) :
  Transaction_base(C, TName),
  m_ID(pqxxInvalidOid),
  m_LogTable()
{
  m_LogTable = string("PQXXLOG_") + Conn().UserName();
  Begin();
}



pqxx::RobustTransaction::~RobustTransaction()
{
  End();
}



void pqxx::RobustTransaction::DoBegin()
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



pqxx::Result pqxx::RobustTransaction::DoExec(const char Query[])
{
  Result R;
  try
  {
    R = DirectExec(Query, 0, SQL_BEGIN_WORK);
  }
  catch (const exception &)
  {
    try { Abort(); } catch (const exception &) { }
    throw;
  }

  return R;
}



void pqxx::RobustTransaction::DoCommit()
{
  const IDType ID = m_ID;

  if (ID == pqxxInvalidOid) 
    throw logic_error("Internal libpqxx error: transaction " 
		      "'" + Name() + "' " 
		     "has no ID");

  try
  {
    DirectExec(SQL_COMMIT_WORK, 0, 0);
  }
  catch (const exception &e)
  {
    m_ID = pqxxInvalidOid;
    if (!Conn().is_open())
    {
      // We've lost the connection while committing.  We'll have to go back to
      // the backend and check our transaction log to see what happened.
      ProcessNotice(e.what() + string("\n"));

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
	ProcessNotice(string(f.what()) + "\n");

        const string Msg = "WARNING: "
		    "Connection lost while committing transaction "
		    "'" + Name() + "' (oid " + ToString(ID) + "). "
		    "Please check for this record in the "
		    "'" + m_LogTable + "' table.  "
		    "If the record exists, the transaction was executed. "
		    "If not, then it hasn't.";

        ProcessNotice(Msg + "\n");
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

  m_ID = pqxxInvalidOid;
  DeleteTransactionRecord(ID);
}


void pqxx::RobustTransaction::DoAbort()
{
  m_ID = pqxxInvalidOid;

  // Rollback transaction.  Our transaction record will be dropped as a side
  // effect, which is what we want since "it never happened."
  DirectExec(SQL_ROLLBACK_WORK, 0, 0);
}


// Create transaction log table if it didn't already exist
void pqxx::RobustTransaction::CreateLogTable()
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


void pqxx::RobustTransaction::CreateTransactionRecord()
{
  const string Insert = "INSERT INTO " + m_LogTable + " "
	                "(name, date) "
			"VALUES "
	                "(" +
	                Quote(Name(), true) + ", "
	                "CURRENT_TIMESTAMP"
	                ")";

  m_ID = DirectExec(Insert.c_str(), 0, 0).InsertedOid();

  if (m_ID == pqxxInvalidOid) 
    throw runtime_error("Could not create transaction log record");
}


void pqxx::RobustTransaction::DeleteTransactionRecord(IDType ID) throw ()
{
  if (ID == pqxxInvalidOid) return;

  try
  {
    // Try very, very hard to delete record.  Specify an absurd retry count to 
    // ensure that the server gets a chance to restart before we give up.
    const string Del = "DELETE FROM " + m_LogTable + " "
	               "WHERE oid=" + ToString(ID);

    DirectExec(Del.c_str(), 20, 0);

    // Now that we've arrived here, we're almost sure that record is quite dead.
    ID = pqxxInvalidOid;
  }
  catch (const exception &e)
  {
  }

  if (ID != pqxxInvalidOid) try
  {
    ProcessNotice("WARNING: "
	          "Failed to delete obsolete transaction record with oid " + 
		  ToString(ID) + " ('" + Name() + "'). "
		  "Please delete it manually.  Thank you.\n");
  }
  catch (const exception &)
  {
  }
}


// Attempt to establish whether transaction record with given ID still exists
bool pqxx::RobustTransaction::CheckTransactionRecord(IDType ID)
{
  const string Find = "SELECT oid FROM " + m_LogTable + " "
	              "WHERE oid=" + ToString(ID);

  return !DirectExec(Find.c_str(), 20, 0).empty();
}

