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

#include "pqxx/connection.h"
#include "pqxx/result.h"
#include "pqxx/robusttransaction.h"


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


// Postfix strings for constructing sequence/index names for table
string Pg::RobustTransaction::s_SeqPostfix("_ID");
string Pg::RobustTransaction::s_IdxPostfix("_IDX");

Pg::RobustTransaction::RobustTransaction(Connection &C, string TName) :
  TransactionItf(C, TName),
  m_ID(0),
  m_LogTable()
{
  m_LogTable = string("PQXXLOG_") + Conn().UserName();
  Begin();
}



Pg::RobustTransaction::~RobustTransaction()
{
  End();
}



void Pg::RobustTransaction::DoBegin()
{
  CreateLogTable();

  // Start backend transaction
  DirectExec(SQL_BEGIN_WORK, 2, 0);

  CreateTransactionRecord();
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
  const IDType ID = m_ID;

  if (!ID) throw logic_error("Internal libpqxx error: transaction " 
		             "'" + Name() + "' " 
			     "has no ID");

  try
  {
    DirectExec(SQL_COMMIT_WORK, 0, 0);
  }
  catch (const exception &e)
  {
    m_ID = 0;
    if (!Conn().IsOpen())
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
		    "'" + Name() + "' (ID " + ToString(ID) + "). "
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

  m_ID = 0;
  DeleteTransactionRecord(ID);
}


void Pg::RobustTransaction::DoAbort()
{
  m_ID = 0;

  // Rollback transaction.  Our transaction record will be dropped as a side
  // effect, which is what we want since "it never happened."
  DirectExec(SQL_ROLLBACK_WORK, 0, 0);
}


// Create transaction log table if it didn't already exist
void Pg::RobustTransaction::CreateLogTable()
{
  // Create log table in case it doesn't already exist.  This code must only be 
  // executed before the backend transaction has properly started.
  const string SeqName = m_LogTable + s_SeqPostfix,
               IdxName = m_LogTable + s_IdxPostfix,
               CrSeq = "CREATE SEQUENCE " + SeqName,
               CrTab = "CREATE TABLE " + m_LogTable +
	               "("
	               "id INTEGER DEFAULT nextval('" + SeqName + "'), "
	               "name VARCHAR(256), "
	               "date TIMESTAMP"
	               ")";

  try { DirectExec(CrSeq.c_str(), 0, 0); } catch (const exception &) { }
  try { DirectExec(CrTab.c_str(), 0, 0); } catch (const exception &) { }
}


void Pg::RobustTransaction::CreateTransactionRecord()
{
  const string MakeID = "SELECT nextval('" + m_LogTable + s_SeqPostfix + "')";
  
  // Get ID for new record
  // TODO: There's a window for leaving garbage here.  Close/document it.
  Result IDR( DirectExec(MakeID.c_str(), 0, 0) );
  IDR.at(0).at(0).to(m_ID);

  const string Insert = "INSERT INTO " + m_LogTable + " "
	                "(id, name, date) "
			"VALUES "
	                "(" +
	                IDR[0][0].c_str() + ", " +
	                Quote(Name(), true) + ", "
	                "CURRENT_TIMESTAMP"
	                ")";

  DirectExec(Insert.c_str(), 0, 0);

  // Zero has a special meaning to us, so don't use the record if its ID is 0.  
  // In that case just leave the zero record in place as a filler.
  if (!m_ID) CreateTransactionRecord();
}


void Pg::RobustTransaction::DeleteTransactionRecord(IDType ID) throw ()
{
  if (!ID) return;

  try
  {
    // Try very, very hard to delete record.  Specify an absurd retry count to 
    // ensure that the server gets a chance to restart before we give up.
    const string Del = "DELETE FROM " + m_LogTable + " "
	               "WHERE id=" + ToString(ID);

    DirectExec(Del.c_str(), 20, 0);

    // Now that we've arrived here, we're sure that record is quite dead.
    ID = 0;
  }
  catch (const exception &e)
  {
  }

  if (ID) try
  {
    ProcessNotice("WARNING: Failed to delete obsolete transaction record " + 
		  ToString(ID) + " ('" + Name() + "'). "
		  "Please delete it manually.  Thank you.\n");

    // TODO: Maintain log of known garbage transaction IDs (failed deletions)?
  }
  catch (const exception &)
  {
  }
}


// Attempt to establish whether transaction record with given ID still exists
bool Pg::RobustTransaction::CheckTransactionRecord(IDType ID)
{
  const string Find = "SELECT id FROM " + m_LogTable + " "
	              "WHERE id=" + ToString(ID);

  return !DirectExec(Find.c_str(), 20, 0).empty();
}

