/** Common code and definitions for the transaction classes.
 *
 * pqxx::transaction_base defines the interface for any abstract class that
 * represents a database transaction.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstring>
#include <stdexcept>

#include "pqxx/connection"
#include "pqxx/result"
#include "pqxx/transaction_base"

#include "pqxx/internal/gates/connection-transaction.hxx"
#include "pqxx/internal/gates/transaction-transactionfocus.hxx"

#include "pqxx/internal/encodings.hxx"


pqxx::transaction_base::transaction_base(connection &C) :
        namedclass{"transaction_base"},
        m_conn{C}
{}


pqxx::transaction_base::~transaction_base()
{
  try
  {
    if (not m_pending_error.empty())
      process_notice("UNPROCESSED ERROR: " + m_pending_error + "\n");

    if (m_registered)
    {
      m_conn.process_notice(description() + " was never closed properly!\n");
      pqxx::internal::gate::connection_transaction{conn()}
        .unregister_transaction(this);
    }
  }
  catch (const std::exception &e)
  {
    try
    {
      process_notice(std::string{e.what()} + "\n");
    }
    catch (const std::exception &)
    {
      process_notice(e.what());
    }
  }
}


void pqxx::transaction_base::register_transaction()
{
  pqxx::internal::gate::connection_transaction{conn()}.register_transaction(
    this);
  m_registered = true;
}


void pqxx::transaction_base::commit()
{
  CheckPendingError();

  // Check previous status code.  Caller should only call this function if
  // we're in "implicit" state, but multiple commits are silently accepted.
  switch (m_status)
  {
  case status::nascent: // We never managed to start the transaction.
    throw usage_error{"Attempt to commit unserviceable " + description() +
                      "."};
    return;

  case status::active: // Just fine.  This is what we expect.
    break;

  case status::aborted:
    throw usage_error{"Attempt to commit previously aborted " + description()};

  case status::committed:
    // Transaction has been committed already.  This is not exactly proper
    // behaviour, but throwing an exception here would only give the impression
    // that an abort is needed--which would only confuse things further at this
    // stage.
    // Therefore, multiple commits are accepted, though under protest.
    m_conn.process_notice(description() + " committed more than once.\n");
    return;

  case status::in_doubt:
    // Transaction may or may not have been committed.  The only thing we can
    // really do is keep telling the caller that the transaction is in doubt.
    throw in_doubt_error{description() +
                         " committed again while in an indeterminate state."};

  default: throw internal_error{"pqxx::transaction: invalid status code."};
  }

  // Tricky one.  If stream is nested in transaction but inside the same scope,
  // the commit() will come before the stream is closed.  Which means the
  // commit is premature.  Punish this swiftly and without fail to discourage
  // the habit from forming.
  if (m_focus.get())
    throw failure{"Attempt to commit " + description() + " with " +
                  m_focus.get()->description() + " still open."};

  // Check that we're still connected (as far as we know--this is not an
  // absolute thing!) before trying to commit.  If the connection was broken
  // already, the commit would fail anyway but this way at least we don't
  // remain in-doubt as to whether the backend got the commit order at all.
  if (not m_conn.is_open())
    throw broken_connection{
      "Broken connection to backend; cannot complete transaction."};

  try
  {
    do_commit();
    m_status = status::committed;
  }
  catch (const in_doubt_error &)
  {
    m_status = status::in_doubt;
    throw;
  }
  catch (const std::exception &)
  {
    m_status = status::aborted;
    throw;
  }

  close();
}


void pqxx::transaction_base::abort()
{
  // Check previous status code.  Quietly accept multiple aborts to
  // simplify emergency bailout code.
  switch (m_status)
  {
  case status::nascent: // Never began transaction.  No need to issue rollback.
    return;

  case status::active:
    try
    {
      do_abort();
    }
    catch (const std::exception &)
    {}
    break;

  case status::aborted: return;

  case status::committed:
    throw usage_error{"Attempt to abort previously committed " +
                      description()};

  case status::in_doubt:
    // Aborting an in-doubt transaction is probably a reasonably sane response
    // to an insane situation.  Log it, but do not complain.
    m_conn.process_notice(
      "Warning: " + description() +
      " aborted after going into "
      "indeterminate state; it may have been executed anyway.\n");
    return;

  default: throw internal_error{"Invalid transaction status."};
  }

  m_status = status::aborted;
  close();
}


std::string pqxx::transaction_base::esc_raw(const std::string &bin) const
{
  const auto p{reinterpret_cast<const unsigned char *>(bin.c_str())};
  return conn().esc_raw(p, bin.size());
}


std::string pqxx::transaction_base::quote_raw(const std::string &bin) const
{
  const auto p{reinterpret_cast<const unsigned char *>(bin.c_str())};
  return conn().quote_raw(p, bin.size());
}


pqxx::result
pqxx::transaction_base::exec(std::string_view Query, const std::string &Desc)
{
  CheckPendingError();

  const std::string N = (Desc.empty() ? "" : "'" + Desc + "' ");

  if (m_focus.get())
    throw usage_error{"Attempt to execute query " + N + "on " + description() +
                      " "
                      "with " +
                      m_focus.get()->description() + " still open."};


  switch (m_status)
  {
  case status::nascent:
    throw usage_error{"Could not execute query " + N +
                      ": "
                      "transaction startup failed."};

  case status::active: break;

  case status::committed:
  case status::aborted:
  case status::in_doubt:
    throw usage_error{"Could not execute query " + N +
                      ": "
                      "transaction is already closed."};

  default: throw internal_error{"pqxx::transaction: invalid status code."};
  }

  // TODO: Pass Desc to direct_exec(), and from there on down
  return direct_exec(Query);
}


pqxx::result pqxx::transaction_base::exec_n(
  result::size_type rows, const std::string &Query, const std::string &Desc)
{
  const result r = exec(Query, Desc);
  if (r.size() != rows)
  {
    const std::string N = (Desc.empty() ? "" : "'" + Desc + "'");
    throw unexpected_rows{"Expected " + to_string(rows) +
                          " row(s) of data "
                          "from query " +
                          N + ", got " + to_string(r.size()) + "."};
  }
  return r;
}


void pqxx::transaction_base::check_rowcount_prepared(
  const std::string &statement, result::size_type expected_rows,
  result::size_type actual_rows)
{
  if (actual_rows != expected_rows)
  {
    throw unexpected_rows{"Expected " + to_string(expected_rows) +
                          " row(s) of data "
                          "from prepared statement '" +
                          statement + "', got " + to_string(actual_rows) +
                          "."};
  }
}


void pqxx::transaction_base::check_rowcount_params(
  size_t expected_rows, size_t actual_rows)
{
  if (actual_rows != expected_rows)
  {
    throw unexpected_rows{"Expected " + to_string(expected_rows) +
                          " row(s) of data "
                          "from parameterised query, got " +
                          to_string(actual_rows) + "."};
  }
}


pqxx::result pqxx::transaction_base::internal_exec_prepared(
  zview statement, const internal::params &args)
{
  return pqxx::internal::gate::connection_transaction{conn()}.exec_prepared(
    statement, args);
}


pqxx::result pqxx::transaction_base::internal_exec_params(
  const std::string &query, const internal::params &args)
{
  return pqxx::internal::gate::connection_transaction{conn()}.exec_params(
    query, args);
}


void pqxx::transaction_base::set_variable(
  std::string_view var, std::string_view value)
{
  conn().set_variable(var, value);
}


std::string pqxx::transaction_base::get_variable(std::string_view var)
{
  return conn().get_variable(var);
}


void pqxx::transaction_base::close() noexcept
{
  try
  {
    try
    {
      CheckPendingError();
    }
    catch (const std::exception &e)
    {
      m_conn.process_notice(e.what());
    }

    if (m_registered)
    {
      m_registered = false;
      pqxx::internal::gate::connection_transaction{conn()}
        .unregister_transaction(this);
    }

    if (m_status != status::active)
      return;

    if (m_focus.get())
      m_conn.process_notice(
        "Closing " + description() + "  with " + m_focus.get()->description() +
        " still open.\n");

    try
    {
      abort();
    }
    catch (const std::exception &e)
    {
      m_conn.process_notice(e.what());
    }
  }
  catch (const std::exception &e)
  {
    try
    {
      m_conn.process_notice(e.what());
    }
    catch (const std::exception &)
    {}
  }
}


void pqxx::transaction_base::register_focus(internal::transactionfocus *S)
{
  m_focus.register_guest(S);
}


void pqxx::transaction_base::unregister_focus(
  internal::transactionfocus *S) noexcept
{
  try
  {
    m_focus.unregister_guest(S);
  }
  catch (const std::exception &e)
  {
    m_conn.process_notice(std::string{e.what()} + "\n");
  }
}


pqxx::result pqxx::transaction_base::direct_exec(std::string_view C)
{
  CheckPendingError();
  return pqxx::internal::gate::connection_transaction{conn()}.exec(C);
}


void pqxx::transaction_base::register_pending_error(
  const std::string &Err) noexcept
{
  if (m_pending_error.empty() and not Err.empty())
  {
    try
    {
      m_pending_error = Err;
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
      {}
    }
  }
}


void pqxx::transaction_base::CheckPendingError()
{
  if (not m_pending_error.empty())
  {
    const std::string Err{m_pending_error};
    m_pending_error.clear();
    throw failure{Err};
  }
}


namespace
{
std::string
MakeCopyString(std::string_view Table, const std::string &Columns)
{
  std::string Q = "COPY " + std::string{Table} + " ";
  if (not Columns.empty())
    Q += "(" + Columns + ") ";
  return Q;
}
} // namespace


void pqxx::transaction_base::BeginCopyRead(
  std::string_view Table, const std::string &Columns)
{
  exec(MakeCopyString(Table, Columns) + "TO STDOUT");
}


void pqxx::transaction_base::BeginCopyWrite(
  std::string_view Table, const std::string &Columns)
{
  exec(MakeCopyString(Table, Columns) + "FROM STDIN");
}


bool pqxx::transaction_base::read_copy_line(std::string &line)
{
  return pqxx::internal::gate::connection_transaction{conn()}.read_copy_line(
    line);
}


void pqxx::transaction_base::write_copy_line(std::string_view line)
{
  pqxx::internal::gate::connection_transaction{conn()}.write_copy_line(line);
}


void pqxx::transaction_base::end_copy_write()
{
  pqxx::internal::gate::connection_transaction{conn()}.end_copy_write();
}


void pqxx::internal::transactionfocus::register_me()
{
  pqxx::internal::gate::transaction_transactionfocus{m_trans}.register_focus(
    this);
  m_registered = true;
}


void pqxx::internal::transactionfocus::unregister_me() noexcept
{
  pqxx::internal::gate::transaction_transactionfocus{m_trans}.unregister_focus(
    this);
  m_registered = false;
}

void pqxx::internal::transactionfocus::reg_pending_error(
  const std::string &err) noexcept
{
  pqxx::internal::gate::transaction_transactionfocus{m_trans}
    .register_pending_error(err);
}
