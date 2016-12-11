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
 * Copyright (c) 2001-2016, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <cstring>
#include <stdexcept>

#include "pqxx/connection_base"
#include "pqxx/result"
#include "pqxx/transaction_base"

#include "pqxx/internal/gates/connection-transaction.hxx"
#include "pqxx/internal/gates/connection-parameterized_invocation.hxx"
#include "pqxx/internal/gates/transaction-transactionfocus.hxx"


using namespace pqxx::internal;


pqxx::internal::parameterized_invocation::parameterized_invocation(
	connection_base &c,
	const std::string &query) :
  m_home(c),
  m_query(query)
{
}


pqxx::result pqxx::internal::parameterized_invocation::exec()
{
  std::vector<const char *> values;
  std::vector<int> lengths;
  std::vector<int> binaries;
  const int elements = marshall(values, lengths, binaries);

  return gate::connection_parameterized_invocation(m_home).parameterized_exec(
	m_query,
	&values[0],
	&lengths[0],
	&binaries[0],
	elements);
}


pqxx::transaction_base::transaction_base(connection_base &C, bool direct) :
  namedclass("transaction_base"),
  m_reactivation_avoidance(),
  m_Conn(C),
  m_Focus(),
  m_Status(st_nascent),
  m_Registered(false),
  m_PendingError()
{
  if (direct)
  {
    gate::connection_transaction gate(conn());
    gate.RegisterTransaction(this);
    m_Registered = true;
  }
}


pqxx::transaction_base::~transaction_base()
{
  try
  {
    reactivation_avoidance_clear();
    if (!m_PendingError.empty())
      process_notice("UNPROCESSED ERROR: " + m_PendingError + "\n");

    if (m_Registered)
    {
      m_Conn.process_notice(description() + " was never closed properly!\n");
      gate::connection_transaction gate(conn());
      gate.UnregisterTransaction(this);
    }
  }
  catch (const std::exception &e)
  {
    try
    {
      process_notice(std::string(e.what()) + "\n");
    }
    catch (const std::exception &)
    {
      process_notice(e.what());
    }
  }
}


void pqxx::transaction_base::commit()
{
  CheckPendingError();

  // Check previous status code.  Caller should only call this function if
  // we're in "implicit" state, but multiple commits are silently accepted.
  switch (m_Status)
  {
  case st_nascent:	// Empty transaction.  No skin off our nose.
    return;

  case st_active:	// Just fine.  This is what we expect.
    break;

  case st_aborted:
    throw usage_error("Attempt to commit previously aborted " + description());

  case st_committed:
    // Transaction has been committed already.  This is not exactly proper
    // behaviour, but throwing an exception here would only give the impression
    // that an abort is needed--which would only confuse things further at this
    // stage.
    // Therefore, multiple commits are accepted, though under protest.
    m_Conn.process_notice(description() + " committed more than once\n");
    return;

  case st_in_doubt:
    // Transaction may or may not have been committed.  The only thing we can
    // really do is keep telling the caller that the transaction is in doubt.
    throw in_doubt_error(description() +
		      " committed again while in an indeterminate state");

  default:
    throw internal_error("pqxx::transaction: invalid status code");
  }

  // Tricky one.  If stream is nested in transaction but inside the same scope,
  // the commit() will come before the stream is closed.  Which means the
  // commit is premature.  Punish this swiftly and without fail to discourage
  // the habit from forming.
  if (m_Focus.get())
    throw failure("Attempt to commit " + description() + " "
		"with " + m_Focus.get()->description() + " "
		"still open");

  // Check that we're still connected (as far as we know--this is not an
  // absolute thing!) before trying to commit.  If the connection was broken
  // already, the commit would fail anyway but this way at least we don't remain
  // in-doubt as to whether the backend got the commit order at all.
  if (!m_Conn.is_open())
    throw broken_connection("Broken connection to backend; "
	"cannot complete transaction");

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
  catch (const std::exception &)
  {
    m_Status = st_aborted;
    throw;
  }

  gate::connection_transaction gate(conn());
  gate.AddVariables(m_Vars);

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
    try { do_abort(); } catch (const std::exception &) { }
    break;

  case st_aborted:
    return;

  case st_committed:
    throw usage_error("Attempt to abort previously committed " + description());

  case st_in_doubt:
    // Aborting an in-doubt transaction is probably a reasonably sane response
    // to an insane situation.  Log it, but do not complain.
    m_Conn.process_notice("Warning: " + description() + " "
		          "aborted after going into indeterminate state; "
			  "it may have been executed anyway.\n");
    return;

  default:
    throw internal_error("invalid transaction status");
  }

  m_Status = st_aborted;
  End();
}


std::string pqxx::transaction_base::esc_raw(const std::string &str) const
{
  const unsigned char *p = reinterpret_cast<const unsigned char *>(str.c_str());
  return conn().esc_raw(p, str.size());
}


std::string pqxx::transaction_base::quote_raw(const std::string &str) const
{
  const unsigned char *p = reinterpret_cast<const unsigned char *>(str.c_str());
  return conn().quote_raw(p, str.size());
}


void pqxx::transaction_base::activate()
{
  switch (m_Status)
  {
  case st_nascent:
    // Make sure transaction has begun before executing anything
    Begin();
    break;

  case st_active:
    break;

  case st_committed:
  case st_aborted:
  case st_in_doubt:
    throw usage_error(
	"Attempt to activate " + description() + " "
	"which is already closed");

  default:
    throw internal_error("pqxx::transaction: invalid status code");
  }
}


pqxx::result pqxx::transaction_base::exec(const std::string &Query,
					  const std::string &Desc)
{
  CheckPendingError();

  const std::string N = (Desc.empty() ? "" : "'" + Desc + "' ");

  if (m_Focus.get())
    throw usage_error("Attempt to execute query " + N +
		      "on " + description() + " "
		      "with " + m_Focus.get()->description() + " "
		      "still open");

  try
  {
    activate();
  }
  catch (const usage_error &e)
  {
    throw usage_error("Error executing query " + N + ".  " + e.what());
  }

  // TODO: Pass Desc to do_exec(), and from there on down
  return do_exec(Query.c_str());
}


pqxx::internal::parameterized_invocation
pqxx::transaction_base::parameterized(const std::string &query)
{
  return internal::parameterized_invocation(conn(), query);
}


pqxx::prepare::invocation
pqxx::transaction_base::prepared(const std::string &statement)
{
  try
  {
    activate();
  }
  catch (const usage_error &e)
  {
    throw usage_error(
	"Error executing prepared statement " + statement + ".  " + e.what());
  }
  return prepare::invocation(*this, statement);
}


void pqxx::transaction_base::set_variable(const std::string &Var,
                                          const std::string &Value)
{
  // Before committing to this new value, see what the backend thinks about it
  gate::connection_transaction gate(conn());
  gate.RawSetVar(Var, Value);
  m_Vars[Var] = Value;
}


std::string pqxx::transaction_base::get_variable(const std::string &Var)
{
  const std::map<std::string,std::string>::const_iterator i = m_Vars.find(Var);
  if (i != m_Vars.end()) return i->second;
  return gate::connection_transaction(conn()).RawGetVar(Var);
}


void pqxx::transaction_base::Begin()
{
  if (m_Status != st_nascent)
    throw internal_error("pqxx::transaction: "
		      "Begin() called while not in nascent state");

  try
  {
    // Better handle any pending notifications before we begin
    m_Conn.get_notifs();

    do_begin();
    m_Status = st_active;
  }
  catch (const std::exception &)
  {
    End();
    throw;
  }
}



void pqxx::transaction_base::End() PQXX_NOEXCEPT
{
  try
  {
    try { CheckPendingError(); }
    catch (const std::exception &e) { m_Conn.process_notice(e.what()); }

    if (m_Registered)
    {
      m_Registered = false;
      gate::connection_transaction gate(conn());
      gate.UnregisterTransaction(this);
    }

    if (m_Status != st_active) return;

    if (m_Focus.get())
      m_Conn.process_notice("Closing " + description() + " "
			    " with " + m_Focus.get()->description() + " "
			    "still open\n");

    try { abort(); }
    catch (const std::exception &e) { m_Conn.process_notice(e.what()); }

    gate::connection_transaction gate(conn());
    gate.take_reactivation_avoidance(m_reactivation_avoidance.get());
    m_reactivation_avoidance.clear();
  }
  catch (const std::exception &e)
  {
    try { m_Conn.process_notice(e.what()); } catch (const std::exception &) {}
  }
}



void pqxx::transaction_base::RegisterFocus(internal::transactionfocus *S)
{
  m_Focus.Register(S);
}


void pqxx::transaction_base::UnregisterFocus(internal::transactionfocus *S)
	PQXX_NOEXCEPT
{
  try
  {
    m_Focus.Unregister(S);
  }
  catch (const std::exception &e)
  {
    m_Conn.process_notice(std::string(e.what()) + "\n");
  }
}


pqxx::result pqxx::transaction_base::DirectExec(const char C[], int Retries)
{
  CheckPendingError();
  return gate::connection_transaction(conn()).Exec(C, Retries);
}


void pqxx::transaction_base::RegisterPendingError(const std::string &Err)
	PQXX_NOEXCEPT
{
  if (m_PendingError.empty() && !Err.empty())
  {
    try
    {
      m_PendingError = Err;
    }
    catch (const std::exception &e)
    {
      try
      {
        process_notice("UNABLE TO PROCESS ERROR\n");
        process_notice(e.what());
        process_notice("ERROR WAS:");
        process_notice(Err);
      }
      catch (...)
      {
      }
    }
  }
}


void pqxx::transaction_base::CheckPendingError()
{
  if (!m_PendingError.empty())
  {
    const std::string Err(m_PendingError);
    m_PendingError.clear();
    throw failure(m_PendingError);
  }
}


namespace
{
std::string MakeCopyString(
        const std::string &Table,
        const std::string &Columns)
{
  std::string Q = "COPY " + Table + " ";
  if (!Columns.empty()) Q += "(" + Columns + ") ";
  return Q;
}
} // namespace


void pqxx::transaction_base::BeginCopyRead(const std::string &Table,
    const std::string &Columns)
{
  exec(MakeCopyString(Table, Columns) + "TO STDOUT");
}


void pqxx::transaction_base::BeginCopyWrite(const std::string &Table,
    const std::string &Columns)
{
  exec(MakeCopyString(Table, Columns) + "FROM STDIN");
}


bool pqxx::transaction_base::ReadCopyLine(std::string &line)
{
  return gate::connection_transaction(conn()).ReadCopyLine(line);
}


void pqxx::transaction_base::WriteCopyLine(const std::string &line)
{
  gate::connection_transaction gate(conn());
  gate.WriteCopyLine(line);
}


void pqxx::transaction_base::EndCopyWrite()
{
  gate::connection_transaction gate(conn());
  gate.EndCopyWrite();
}


void pqxx::internal::transactionfocus::register_me()
{
  gate::transaction_transactionfocus gate(m_Trans);
  gate.RegisterFocus(this);
  m_registered = true;
}


void pqxx::internal::transactionfocus::unregister_me() PQXX_NOEXCEPT
{
  gate::transaction_transactionfocus gate(m_Trans);
  gate.UnregisterFocus(this);
  m_registered = false;
}

void
pqxx::internal::transactionfocus::reg_pending_error(const std::string &err)
	PQXX_NOEXCEPT
{
  gate::transaction_transactionfocus gate(m_Trans);
  gate.RegisterPendingError(err);
}
