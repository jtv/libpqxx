/*-------------------------------------------------------------------------
 *
 *   FILE
 *	transaction_base.cxx
 *
 *   DESCRIPTION
 *      common code and definitions for the transaction classes
 *   pqxx::transaction_base defines the interface for any abstract class that
 *   represents a database transaction
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
#include "pqxx/tablestream"
#include "pqxx/transaction_base"


using namespace PGSTD;


pqxx::transaction_base::transaction_base(connection_base &C, 
    					 const string &TName) :
  m_Conn(C),
  m_Name(TName),
  m_UniqueCursorNum(1),
  m_Stream(),
  m_Status(st_nascent),
  m_Registered(false)
{
  m_Conn.RegisterTransaction(this);
  m_Registered = true;
}


pqxx::transaction_base::~transaction_base()
{
  if (m_Registered)
  {
    m_Conn.process_notice("Transaction '" + m_Name + "' "
		          "was never closed properly!\n");
    m_Conn.UnregisterTransaction(this);
  }
}


void pqxx::transaction_base::commit()
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
		      name() +
		      "'");

  case st_committed:
    // Transaction has been committed already.  This is not exactly proper 
    // behaviour, but throwing an exception here would only give the impression
    // that an abort is needed--which would only confuse things further at this
    // stage.
    // Therefore, multiple commits are accepted, though under protest.
    m_Conn.process_notice("Transaction '" + 
		          name() + 
			  "' committed more than once\n");
    return;

  case st_in_doubt:
    // Transaction may or may not have been committed.  Report the problem but
    // don't compound our troubles by throwing.
    throw logic_error("Transaction '" + name() + "' "
		      "committed again while in an undetermined state\n");

  default:
    throw logic_error("Internal libpqxx error: pqxx::transaction: invalid status code");
  }
 
  // Tricky one.  If stream is nested in transaction but inside the same scope,
  // the Commit() will come before the stream is closed.  Which means the
  // commit is premature.  Punish this swiftly and without fail to discourage
  // the habit from forming.
  if (m_Stream.get())
    throw runtime_error("Attempt to commit transaction '" + 
		        name() +
			"' with stream '" +
			m_Stream.get()->name() + 
			"' still open");

  try
  {
    do_commit();
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

  m_Conn.AddVariables(m_Vars);

  End();
}


void pqxx::transaction_base::abort()
{
  // Check previous status code.  Quietly accept multiple aborts to 
  // simplify emergency bailout code.
  switch (m_Status)
  {
  case st_nascent:	// Never began transaction.  No need to issue rollback.
    break;

  case st_active:
    try { do_abort(); } catch (const exception &) { }
    break;

  case st_aborted:
    return;

  case st_committed:
    throw logic_error("Attempt to abort previously committed transaction '" +
		      name() +
		      "'");

  case st_in_doubt:
    // Aborting an in-doubt transaction is probably a reasonably sane response
    // to an insane situation.  Log it, but do not complain.
    m_Conn.process_notice("Warning: Transaction '" + name() + "' "
		          "aborted after going into indeterminate state; "
			  "it may have been executed anyway.\n");
    return;

  default:
    throw logic_error("Internal libpqxx error: pqxx::transaction: invalid status code");
  }

  m_Status = st_aborted;
  End();
}


pqxx::result pqxx::transaction_base::exec(const char Query[],
    					const string &Desc)
{
  const string N = (Desc.empty() ? "" : "'" + Desc + "'");

  if (m_Stream.get())
    throw logic_error("Attempt to execute query '" + N + " on transaction '" + 
		      name() + 
		      "' while stream '" +
		      m_Stream.get()->name() +
		      "' is still open");

  switch (m_Status)
  {
  case st_nascent:
    // Make sure transaction has begun before executing anything
    Begin();
    break;

  case st_active:
    break;

  case st_committed:
    throw logic_error("Attempt to execute query " + N + " "
	              "in committed transaction '" +
		      name() +
		      "'");

  case st_aborted:
    throw logic_error("Attempt to execute query " + N + " "
	              "in aborted transaction '" +
		      name() +
		      "'");

  case st_in_doubt:
    throw logic_error("Attempt to execute query " + N + " in transaction '" + 
		      name() + 
		      "', which is in indeterminate state");
  default:
    throw logic_error("Internal libpqxx error: pqxx::transaction: "
		      "invalid status code");
  }

  // TODO: Pass Name to DoExec(), and from there on down
  return do_exec(Query);
}


void pqxx::transaction_base::set_variable(const PGSTD::string &Var,
                                          const PGSTD::string &Value)
{
  m_Conn.RawSetVar(Var, Value);
  m_Vars[Var] = Value;
}


void pqxx::transaction_base::Begin()
{
  if (m_Status != st_nascent)
    throw logic_error("Internal libpqxx error: pqxx::transaction: "
		      "Begin() called while not in nascent state");

  try
  {
    // Better handle any pending notifications before we begin
    m_Conn.get_notifs();

    do_begin();
    m_Status = st_active;
  }
  catch (const exception &)
  {
    End();
    throw;
  }
}



void pqxx::transaction_base::End() throw ()
{
  if (!m_Registered) return;

  try
  {
    m_Conn.UnregisterTransaction(this);
    m_Registered = false;

    if (m_Stream.get())
      m_Conn.process_notice("Closing transaction '" +
		            name() +
			    "' with stream '" +
			    m_Stream.get()->name() + 
			    "' still open\n");

    if (m_Status == st_active) abort();
  }
  catch (const exception &e)
  {
    m_Conn.process_notice(string(e.what()) + "\n");
  }
}



void pqxx::transaction_base::RegisterStream(tablestream *S)
{
  m_Stream.Register(S);
}


void pqxx::transaction_base::UnregisterStream(tablestream *S) throw ()
{
  try
  {
    m_Stream.Unregister(S);
  }
  catch (const exception &e)
  {
    m_Conn.process_notice(string(e.what()) + "\n");
  }
}


pqxx::result pqxx::transaction_base::DirectExec(const char C[], 
		                      int Retries,
				      const char OnReconnect[])
{
  return m_Conn.Exec(C, Retries, OnReconnect);
}

