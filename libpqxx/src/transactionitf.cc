/*-------------------------------------------------------------------------
 *
 *   FILE
 *	transactionitf.cc
 *
 *   DESCRIPTION
 *      common code and definitions for the transaction classes
 *   Pg::TransactionItf defines the interface for any abstract class that
 *   represents a database transaction
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pqxx/connection.h"
#include "pqxx/result.h"
#include "pqxx/tablestream.h"
#include "pqxx/transactionitf.h"


using namespace PGSTD;


Pg::TransactionItf::TransactionItf(Connection &C, string TName) :
  m_Conn(C),
  m_Status(st_nascent),
  m_Name(TName),
  m_UniqueCursorNum(1),
  m_Stream()
{
  m_Conn.RegisterTransaction(this);
}


Pg::TransactionItf::~TransactionItf()
{
}


void Pg::TransactionItf::Commit()
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

  try
  {
    DoCommit();
    m_Status = st_committed;
  }
  catch (const in_doubt_error &)
  {
    m_Status = st_in_doubt;
    throw;
  }
  catch (const exception &)
  {
    m_Status = st_aborted;
    throw;
  }
}


void Pg::TransactionItf::Abort()
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
      DoAbort();
    }
    catch (const exception &)
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


Pg::Result Pg::TransactionItf::Exec(const char C[])
{
  if (m_Stream.get())
    throw logic_error("Attempt to execute query on transaction '" + 
		      Name() + 
		      "' while stream still open");

  switch (m_Status)
  {
  case st_nascent:
    // Make sure transaction has begun before executing anything
    Begin();
    break;

  case st_active:
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

  return DoExec(C);
}



void Pg::TransactionItf::Begin()
{
  if (m_Status != st_nascent)
    throw logic_error("Internal libpqxx error: Pg::Transaction: "
		      "Begin() called while not in nascent state");

  // Better handle any pending notifications before we begin
  m_Conn.GetNotifs();

  DoBegin();
  m_Status = st_active;
}



void Pg::TransactionItf::End()
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



void Pg::TransactionItf::RegisterStream(const TableStream *S)
{
  m_Stream.Register(S);
}


void Pg::TransactionItf::UnregisterStream(const TableStream *S) throw ()
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


Pg::Result Pg::TransactionItf::DirectExec(const char C[], 
		                      int Retries,
				      const char OnReconnect[])
{
  return m_Conn.Exec(C, Retries, OnReconnect);
}

