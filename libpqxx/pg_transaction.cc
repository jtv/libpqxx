/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_transaction.cc
 *
 *   DESCRIPTION
 *      implementation of the Pg::Transaction class.
 *   Pg::Transaction represents a database transaction
 *
 * Copyright (c) 2001, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
  m_Conn.UnregisterTransaction(this);

  if (m_Status == st_committed)
  {
    m_Conn.Exec(SQL_COMMIT_WORK, 0);
  }
  else
  {
    try
    {
      Abort();
    }
    catch (const exception &e)
    {
      m_Conn.ProcessNotice(string(e.what()) + "\n");
    }
  }

  if (m_Stream.get())
    m_Conn.ProcessNotice("Closing transaction '" +
		         Name() +
			 "' with stream still open\n");
}


void Pg::Transaction::Commit()
{
  // Check previous status code.  Caller should only call this function if
  // we're in "implicit" state, but multiple commits are silently accepted.
  switch (m_Status)
  {
  case st_nascent:	// Empty transaction.  No skin off our nose.
  case st_active:	// Just fine.  This is what we expect.
    break;

  case st_aborted:
    throw logic_error("Attempt to commit previously aborted database transaction");

  case st_committed:
    // Transaction has been committed already.  This is not exactly proper 
    // behaviour, but throwing an exception here would only give the impression
    // that an abort is needed--which would only confuse things further at this
    // stage.
    // Therefore, multiple commits are silently accepted.
    m_Conn.ProcessNotice("Transaction '" + 
		         Name() + 
			 "' committed more than once\n");
    break;

  default:
    throw logic_error("Internal libpqxx error: Pg::Transaction: invalid status code");
  }

  m_Status = st_committed;
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
    throw logic_error("Attempt to abort previously committed transaction");

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
    throw logic_error("Attempt to execute query in committed transaction");

  case st_aborted:
    throw logic_error("Attempt to execute query in aborted transaction");

  default:
    throw logic_error("Internal libpqxx error: Pg::Transaction: invalid status code");
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


