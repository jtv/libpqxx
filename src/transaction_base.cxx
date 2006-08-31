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
 * Copyright (c) 2001-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
#include "pqxx/tablestream"
#include "pqxx/transaction_base"


using namespace PGSTD;
using namespace pqxx::internal;


pqxx::transaction_base::transaction_base(connection_base &C, bool direct) :
  namedclass("transaction_base"),
  m_Conn(C),
  m_Focus(),
  m_Status(st_nascent),
  m_Registered(false),
  m_PendingError(),
  m_reactivation_avoidance()
{
  if (direct)
  {
    m_Conn.RegisterTransaction(this);
    m_Registered = true;
  }
}


pqxx::transaction_base::~transaction_base()
{
#ifdef PQXX_QUIET_DESTRUCTORS
  disable_noticer Quiet(m_Conn);
#endif
  try
  {
    reactivation_avoidance_clear();
    if (!m_PendingError.empty())
      process_notice("UNPROCESSED ERROR: " + m_PendingError + "\n");

    if (m_Registered)
    {
      m_Conn.process_notice(description() + " was never closed properly!\n");
      m_Conn.UnregisterTransaction(this);
    }
  }
  catch (const exception &e)
  {
    try
    {
      process_notice(string(e.what()) + "\n");
    }
    catch (const exception &)
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
    throw logic_error("Attempt to commit previously aborted " + description());

  case st_committed:
    // Transaction has been committed already.  This is not exactly proper
    // behaviour, but throwing an exception here would only give the impression
    // that an abort is needed--which would only confuse things further at this
    // stage.
    // Therefore, multiple commits are accepted, though under protest.
    m_Conn.process_notice(description() + " committed more than once\n");
    return;

  case st_in_doubt:
    // Transaction may or may not have been committed.  Report the problem but
    // don't compound our troubles by throwing.
    throw logic_error(description() +
		      "committed again while in an indeterminate state");

  default:
    throw internal_error("pqxx::transaction: invalid status code");
  }

  // Tricky one.  If stream is nested in transaction but inside the same scope,
  // the Commit() will come before the stream is closed.  Which means the
  // commit is premature.  Punish this swiftly and without fail to discourage
  // the habit from forming.
  if (m_Focus.get())
    throw runtime_error("Attempt to commit " + description() + " "
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
    throw logic_error("Attempt to abort previously committed " + description());

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


string pqxx::transaction_base::esc(const char str[]) const
{
  return m_Conn.esc(str, strlen(str));
}


string pqxx::transaction_base::esc(const PGSTD::string &str) const
{
  return m_Conn.esc(str.c_str(), str.size());
}


string pqxx::transaction_base::esc_raw(const PGSTD::string &str) const
{
  const unsigned char *p = reinterpret_cast<const unsigned char *>(str.c_str());
  return m_Conn.esc_raw(p, str.size());
}


pqxx::result pqxx::transaction_base::exec(const char Query[],
					  const PGSTD::string &Desc)
{
  CheckPendingError();

  const string N = (Desc.empty() ? "" : "'" + Desc + "' ");

  if (m_Focus.get())
    throw logic_error("Attempt to execute query " + N +
		      "on " + description() + " "
		      "with " + m_Focus.get()->description() + " "
		      "still open");

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
    throw logic_error("Attempt to execute query " + N + " "
	"in " + description() + ", which is already closed");

  default:
    throw internal_error("pqxx::transaction: invalid status code");
  }

  // TODO: Pass Desc to do_exec(), and from there on down
  return do_exec(Query);
}


pqxx::prepare::invocation
pqxx::transaction_base::prepared(const PGSTD::string &statement)
{
  return prepare::invocation(*this, statement);
}


pqxx::result pqxx::transaction_base::prepared_exec(
	const PGSTD::string &statement,
	const char *const params[],
	const int paramlengths[],
	int nparams)
{
  return m_Conn.prepared_exec(statement, params, paramlengths, nparams);
}


void pqxx::transaction_base::set_variable(const PGSTD::string &Var,
                                          const PGSTD::string &Value)
{
  // Before committing to this new value, see what the backend thinks about it
  m_Conn.RawSetVar(Var, Value);
  m_Vars[Var] = Value;
}


string pqxx::transaction_base::get_variable(const PGSTD::string &Var)
{
  const map<string,string>::const_iterator i = m_Vars.find(Var);
  if (i != m_Vars.end()) return i->second;
  return m_Conn.RawGetVar(Var);
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
  catch (const exception &)
  {
    End();
    throw;
  }
}



void pqxx::transaction_base::End() throw ()
{
  try
  {
    try { CheckPendingError(); }
    catch (const exception &e) { m_Conn.process_notice(e.what()); }

    if (m_Registered)
    {
      m_Registered = false;
      m_Conn.UnregisterTransaction(this);
    }

    if (m_Status != st_active) return;

    if (m_Focus.get())
      m_Conn.process_notice("Closing " + description() + " "
			    " with " + m_Focus.get()->description() + " "
			    "still open\n");

    try { abort(); }
    catch (const exception &e) { m_Conn.process_notice(e.what()); }

    const int ra = m_reactivation_avoidance.get();
    if (ra)
    {
      reactivation_avoidance_clear();
      conn().m_reactivation_avoidance.add(ra);
    }
  }
  catch (const exception &e)
  {
    try { m_Conn.process_notice(e.what()); } catch (const exception &) {}
  }
}



void pqxx::transaction_base::RegisterFocus(internal::transactionfocus *S)
{
  m_Focus.Register(S);
}


void pqxx::transaction_base::UnregisterFocus(internal::transactionfocus *S)
	throw ()
{
  try
  {
    m_Focus.Unregister(S);
  }
  catch (const exception &e)
  {
    m_Conn.process_notice(string(e.what()) + "\n");
  }
}


pqxx::result pqxx::transaction_base::DirectExec(const char C[], int Retries)
{
  CheckPendingError();
  return m_Conn.Exec(C, Retries);
}


void pqxx::transaction_base::RegisterPendingError(const PGSTD::string &Err)
	throw ()
{
  if (m_PendingError.empty() && !Err.empty())
  {
    try
    {
      m_PendingError = Err;
    }
    catch (const exception &e)
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
    const string Err(m_PendingError);
#ifdef PQXX_HAVE_STRING_CLEAR
    m_PendingError.clear();
#else
    m_PendingError.resize(0);
#endif
    throw runtime_error(m_PendingError);
  }
}


namespace
{
string MakeCopyString(const string &Table, const string &Columns)
{
  string Q = "COPY " + Table + " ";
  if (!Columns.empty()) Q += "(" + Columns + ") ";
  return Q;
}
} // namespace


void pqxx::transaction_base::BeginCopyRead(const PGSTD::string &Table,
    const PGSTD::string &Columns)
{
  exec(MakeCopyString(Table, Columns) + "TO STDOUT");
}


void pqxx::transaction_base::BeginCopyWrite(const PGSTD::string &Table,
    const PGSTD::string &Columns)
{
  exec(MakeCopyString(Table, Columns) + "FROM STDIN");
}


void pqxx::internal::transactionfocus::register_me()
{
  m_Trans.RegisterFocus(this);
  m_registered = true;
}


void pqxx::internal::transactionfocus::unregister_me() throw ()
{
  m_Trans.UnregisterFocus(this);
  m_registered = false;
}

void
pqxx::internal::transactionfocus::reg_pending_error(const PGSTD::string &err)
	throw ()
{
  m_Trans.RegisterPendingError(err);
}


