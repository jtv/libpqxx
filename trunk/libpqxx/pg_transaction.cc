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


Pg::Transaction::Transaction(Connection &Conn, string TName) :
  m_Conn(Conn),
  m_Status(st_nascent),
  m_Name(TName),
  m_UniqueCursorNum(1),
  m_Stream()
{
  m_Conn.RegisterTransaction(this);
  Begin();
}


Pg::Transaction::~Transaction()
{
  try
  {
    m_Conn.UnregisterTransaction(this);

    if (m_Status == st_active) Abort();

    if (m_Stream.get())
      m_Conn.ProcessNotice("Closing transaction '" +
		           Name() +
			   "' with stream '" +
			   m_Stream.get()->Name() + 
			   "' still open\n");
  }
  catch (const exception &e)
  {
    m_Conn.ProcessNotice(string(e.what()) + "\n");
  }
}


void Pg::Transaction::Commit()
{
  // Check previous status code.  Caller should only call this function if
  // we're in "implicit" state, but multiple commits are silently accepted.
  switch (m_Status)
  {
  case st_nascent:	// Empty transaction.  No skin off our nose.
    return;

  case st_active:	// Just fine.  This is what we expect.
    break;

  case st_aborted:
    throw logic_error("Attempt to commit previously aborted transaction '" +
		      Name() +
		      "'");

  case st_committed:
    // Transaction has been committed already.  This is not exactly proper 
    // behaviour, but throwing an exception here would only give the impression
    // that an abort is needed--which would only confuse things further at this
    // stage.
    // Therefore, multiple commits are accepted, though under protest.
    m_Conn.ProcessNotice("Transaction '" + 
		         Name() + 
			 "' committed more than once\n");
    return;

  case st_in_doubt:
    // Transaction may or may not have been committed.  Report the problem but
    // don't compound our troubles by throwing.
    throw logic_error("Transaction '" + Name() + "' "
		      "committed again while in an undetermined state\n");

  default:
    throw logic_error("Internal libpqxx error: Pg::Transaction: invalid status code");
  }
 
  // Tricky one.  If stream is nested in transaction but inside the same scope,
  // the Commit() will come before the stream is closed.  Which means the
  // commit is premature.  Punish this swiftly and without fail to discourage
  // the habit from forming.
  if (m_Stream.get())
    throw runtime_error("Attempt to commit transaction '" + 
		        Name() +
			"' with stream '" +
			m_Stream.get()->Name() + 
			"' still open");

  // We're "in doubt" until we're sure the commit succeeds, or it fails without
  // losing the connection.
  m_Status = st_in_doubt;

  try
  {
    m_Conn.Exec(SQL_COMMIT_WORK, 0);
    m_Status = st_committed;
  }
  catch (const exception &e)
  {
    if (!m_Conn.IsOpen())
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
      m_Status = st_aborted;
      throw;
    }
  }
}


void Pg::Transaction::Abort()
{
  // Check previous status code.  Quietly accept multiple aborts to 
  // simplify emergency bailout code.
  switch (m_Status)
  {
  case st_nascent:	// Never began transaction.  No need to issue rollback.
    break;

  case st_active:
    try
    {
      m_Conn.Exec(SQL_ROLLBACK_WORK, 0);
    }
    catch (...)
    {
    }
    break;

  case st_aborted:
    return;

  case st_committed:
    throw logic_error("Attempt to abort previously committed transaction '" +
		      Name() +
		      "'");

  case st_in_doubt:
    // Aborting an in-doubt transaction is probably a reasonably sane response
    // to an insane situation.  Log it, but do not complain.
    m_Conn.ProcessNotice("Warning: Transaction '" + Name() + "' "
		         "aborted after going into indeterminate state; "
			 "it may have been executed anyway.\n");
    return;

  default:
    throw logic_error("Internal libpqxx error: Pg::Transaction: invalid status code");
  }

  m_Status = st_aborted;
}


Pg::Result Pg::Transaction::Exec(const char C[])
{
  if (m_Stream.get())
    throw logic_error("Attempt to execute query on transaction '" + 
		      Name() + 
		      "' while stream still open");

  int Retries;

  switch (m_Status)
  {
  case st_nascent:
    Begin();
    Retries = 2;
    break;

  case st_active:
    Retries = 0;
    break;

  case st_committed:
    throw logic_error("Attempt to execute query in committed transaction '" +
		      Name() +
		      "'");

  case st_aborted:
    throw logic_error("Attempt to execute query in aborted transaction '" +
		      Name() +
		      "'");

  case st_in_doubt:
    throw logic_error("Attempt to execute query in transaction '" + 
		      Name() + 
		      "', which is in indeterminate state");
  default:
    throw logic_error("Internal libpqxx error: Pg::Transaction: "
		      "invalid status code");
  }

  Result R;
  try
  {
    R = m_Conn.Exec(C, Retries, SQL_BEGIN_WORK);
  }
  catch (...)
  {
    Abort();
    throw;
  }

  return R;
}



void Pg::Transaction::Begin()
{
  if (m_Status != st_nascent)
    throw logic_error("Internal libpqxx error: Pg::Transaction: "
		      "Begin() called while not in nascent state");

  // Better handle any pending notifications before we begin
  m_Conn.GetNotifs();

  // Now start transaction
  m_Conn.Exec(SQL_BEGIN_WORK);
  m_Status = st_active;
}


void Pg::Transaction::RegisterStream(const TableStream *S)
{
  m_Stream.Register(S);
}


void Pg::Transaction::UnregisterStream(const TableStream *S) throw ()
{
  try
  {
    m_Stream.Unregister(S);
  }
  catch (const exception &e)
  {
    m_Conn.ProcessNotice(string(e.what()) + "\n");
  }
}


