/** Common code and definitions for the transaction classes.
 *
 * pqxx::transaction_base defines the interface for any abstract class that
 * represents a database transaction.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstring>
#include <stdexcept>
#include <utility>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/connection.hxx"
#include "pqxx/internal/encodings.hxx"
#include "pqxx/internal/gates/connection-transaction.hxx"
#include "pqxx/internal/gates/transaction-transaction_focus.hxx"
#include "pqxx/result.hxx"
#include "pqxx/transaction_base.hxx"
#include "pqxx/transaction_focus.hxx"

#include "pqxx/internal/header-post.hxx"


using namespace std::literals;

namespace
{
/// Return a query pointer for the command "ROLLBACK".
/** Concentrates constructions so as to minimise the number of allocations.
 * This way, the string gets allocated once and then all subsequent invocations
 * copy shared_ptr instances to the same string.
 */
std::shared_ptr<std::string> make_rollback_cmd()
{
  static auto const cmd{std::make_shared<std::string>("ROLLBACK")};
  return cmd;
}
} // namespace

pqxx::transaction_base::transaction_base(connection &cx, sl loc) :
        m_conn{cx}, m_rollback_cmd{make_rollback_cmd()}, m_created_loc{loc}
{}


pqxx::transaction_base::transaction_base(
  connection &cx, std::string_view tname, sl loc) :
        m_conn{cx},
        m_name{tname},
        m_rollback_cmd{make_rollback_cmd()},
        m_created_loc{loc}
{}


pqxx::transaction_base::transaction_base(
  connection &cx, std::string_view tname,
  std::shared_ptr<std::string> rollback_cmd, sl loc) :
        m_conn{cx},
        m_name{tname},
        m_rollback_cmd{std::move(rollback_cmd)},
        m_created_loc{loc}
{}


pqxx::transaction_base::~transaction_base()
{
  try
  {
    if (not std::empty(m_pending_error)) [[unlikely]]
      process_notice(std::format("UNPROCESSED ERROR: {}\n", m_pending_error));

    if (m_registered)
    {
      m_conn.process_notice(
        std::format("{} was never closed properly!\n", description()));
      pqxx::internal::gate::connection_transaction{conn()}
        .unregister_transaction(this);
    }
  }
  catch (std::exception const &e)
  {
    try
    {
      process_notice(std::format("{}\n", e.what()));
    }
    catch (std::exception const &)
    {
      // TODO: Make at least an attempt to append a newline.
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


pqxx::conversion_context pqxx::transaction_base::make_context(sl loc) const
{
  return conversion_context{m_conn.get_encoding_group(loc), loc};
}


void pqxx::transaction_base::commit(sl loc)
{
  check_pending_error();

  // Check previous status code.  Caller should only call this function if
  // we're in "implicit" state, but multiple commits are silently accepted.
  switch (m_status)
  {
  case status::active: // Just fine.  This is what we expect.
    break;

  case status::aborted:
    throw usage_error{
      std::format("Attempt to commit previously aborted {}", description()),
      loc};

  case status::committed:
    // Transaction has been committed already.  This is not exactly proper
    // behaviour, but throwing an exception here would only give the impression
    // that an abort is needed--which would only confuse things further at this
    // stage.
    // Therefore, multiple commits are accepted, though under protest.
    m_conn.process_notice(
      std::format("{} committed more than once.\n", description()));
    return;

  case status::in_doubt:
    // Transaction may or may not have been committed.  The only thing we can
    // really do is keep telling the caller that the transaction is in doubt.
    throw in_doubt_error{std::format(
      "{} committed again while in an indeterminate state.", description())};

  default: PQXX_UNREACHABLE;
  }

  // Tricky one.  If stream is nested in transaction but inside the same scope,
  // the commit() will come before the stream is closed.  Which means the
  // commit is premature.  Punish this swiftly and without fail to discourage
  // the habit from forming.
  if (m_focus != nullptr)
    throw failure{
      std::format(
        "Attempt to commit {} with {} still open.", description(),
        m_focus->description()),
      loc};

  // Check that we're still connected (as far as we know--this is not an
  // absolute thing!) before trying to commit.  If the connection was broken
  // already, the commit would fail anyway but this way at least we don't
  // remain in-doubt as to whether the backend got the commit order at all.
  if (not m_conn.is_open())
    throw broken_connection{
      "Broken connection to backend; cannot complete transaction.", loc};

  try
  {
    do_commit(loc);
    m_status = status::committed;
  }
  catch (in_doubt_error const &)
  {
    m_status = status::in_doubt;
    throw;
  }
  catch (std::exception const &)
  {
    m_status = status::aborted;
    throw;
  }

  close(loc);
}


void pqxx::transaction_base::do_abort(sl loc)
{
  if (m_rollback_cmd)
    direct_exec(m_rollback_cmd, loc);
}


void pqxx::transaction_base::abort(sl loc)
{
  // Check previous status code.  Quietly accept multiple aborts to
  // simplify emergency bailout code.
  switch (m_status)
  {
  case status::active:
    try
    {
      do_abort(loc);
    }
    catch (std::exception const &e)
    {
      m_conn.process_notice(std::format("{}\n", e.what()));
    }
    break;

  case status::aborted: return;

  case status::committed:
    throw usage_error{
      std::format("Attempt to abort previously committed {}.", description()),
      loc};

  case status::in_doubt:
    // Aborting an in-doubt transaction is probably a reasonably sane response
    // to an insane situation.  Log it, but do not fail.
    m_conn.process_notice(std::format(
      "Warning: {} aborted after going into indeterminate state; "
      "it may have been executed anyway.\n",
      description()));
    return;

  default: PQXX_UNREACHABLE;
  }

  m_status = status::aborted;
  close(loc);
}


namespace
{
/// Guard command execution against clashes with pipelines and such.
/** A transaction can have only one focus at a time.  Command execution is the
 * most basic example of a transaction focus.
 */
class PQXX_PRIVATE command : pqxx::transaction_focus
{
public:
  command(pqxx::transaction_base &tx, std::string_view oname) :
          transaction_focus{tx, "command"sv, oname}
  {
    register_me();
  }

  ~command() noexcept { unregister_me(); }

  command() = delete;
  command(command const &) = delete;
  command(command &&) = delete;
  command &operator=(command const &) = delete;
  command &operator=(command &&) = delete;
};
} // namespace

pqxx::result pqxx::transaction_base::exec(std::string_view query, sl loc)
{
  check_pending_error();

  command const cmd{*this, {}};

  switch (m_status)
  {
  case status::active: break;

  case status::committed:
  case status::aborted:
  case status::in_doubt:
    // TODO: Pass query.
    throw usage_error{
      "Could not execute command: transaction is already closed.", loc};

  default: PQXX_UNREACHABLE;
  }

  return direct_exec(query, loc);
}


pqxx::result pqxx::transaction_base::exec(
  std::string_view query, std::string_view desc, sl loc)
{
  check_pending_error();

  command const cmd{*this, desc};

  switch (m_status)
  {
  case status::active: break;

  case status::committed:
  case status::aborted:
  case status::in_doubt: {
    std::string const n{std::empty(desc) ? "" : std::format("'{}' ", desc)};

    throw usage_error{
      std::format(
        "Could not execute command {}: transaction is already closed.", n),
      loc};
  }

  default: PQXX_UNREACHABLE;
  }

  return direct_exec(query, desc, loc);
}


pqxx::result pqxx::transaction_base::exec_n(
  result::size_type rows, std::string_view query, std::string_view desc)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  result r{exec(query, desc)};
#include "pqxx/internal/ignore-deprecated-post.hxx"
  r.expect_rows(rows);
  return r;
}


pqxx::result pqxx::transaction_base::internal_exec_prepared(
  std::string_view statement, internal::c_params const &args, sl loc)
{
  command const cmd{*this, statement};
  return pqxx::internal::gate::connection_transaction{conn()}.exec_prepared(
    statement, args, loc);
}


pqxx::result pqxx::transaction_base::internal_exec_params(
  std::string_view query, internal::c_params const &args, sl loc)
{
  command const cmd{*this, query};
  return pqxx::internal::gate::connection_transaction{conn()}.exec_params(
    query, args, loc);
}


void pqxx::transaction_base::notify(
  std::string_view channel, std::string_view payload, sl loc)
{
  // For some reason, NOTIFY does not work as a parameterised statement,
  // even just for the payload (which is supposed to be a normal string).
  // Luckily, pg_notify() does.
  exec("SELECT pg_notify($1, $2)", params{*this, channel, payload}, loc)
    .one_row(loc);
}


void pqxx::transaction_base::set_variable(
  std::string_view var, std::string_view value)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  conn().set_variable(var, value);
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


std::string pqxx::transaction_base::get_variable(std::string_view var)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  return conn().get_variable(var);
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void pqxx::transaction_base::close(sl loc) noexcept
{
  try
  {
    try
    {
      check_pending_error();
    }
    catch (std::exception const &e)
    {
      // TODO: Make at least an attempt to append a newline.
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

    if (m_focus != nullptr) [[unlikely]]
      m_conn.process_notice(std::format(
        "Closing {} with {} still open.\n", description(),
        m_focus->description()));

    try
    {
      abort(loc);
    }
    catch (std::exception const &e)
    {
      // TODO: Make at least an attempt to append a newline.
      m_conn.process_notice(e.what());
    }
  }
  catch (std::exception const &e)
  {
    try
    {
      // TODO: Make at least an attempt to append a newline.
      m_conn.process_notice(e.what());
    }
    catch (std::exception const &)
    {}
  }
}


namespace
{
[[nodiscard]] std::string_view
get_classname(pqxx::transaction_focus const *focus)
{
  return (focus == nullptr) ? ""sv : focus->classname();
}


[[nodiscard]] std::string_view
get_obj_name(pqxx::transaction_focus const *focus)
{
  return (focus == nullptr) ? ""sv : focus->name();
}
} // namespace


void pqxx::transaction_base::register_focus(transaction_focus *new_focus)
{
  internal::check_unique_register(
    m_focus, get_classname(m_focus), get_obj_name(m_focus), new_focus,
    get_classname(new_focus), get_obj_name(new_focus));
  m_focus = new_focus;
}


void pqxx::transaction_base::unregister_focus(
  transaction_focus *new_focus) noexcept
{
  try
  {
    pqxx::internal::check_unique_unregister(
      m_focus, get_classname(m_focus), get_obj_name(m_focus), new_focus,
      get_classname(new_focus), get_obj_name(new_focus));
    m_focus = nullptr;
  }
  catch (std::exception const &e)
  {
    // TODO: Make at least an attempt to append a newline.
    m_conn.process_notice(e.what());
  }
}


pqxx::result pqxx::transaction_base::direct_exec(
  std::string_view cmd, std::string_view desc, sl loc)
{
  check_pending_error();
  return pqxx::internal::gate::connection_transaction{conn()}.exec(
    cmd, desc, loc);
}


pqxx::result pqxx::transaction_base::direct_exec(
  std::shared_ptr<std::string> cmd, std::string_view desc, sl loc)
{
  check_pending_error();
  return pqxx::internal::gate::connection_transaction{conn()}.exec(
    std::move(cmd), desc, loc);
}


void pqxx::transaction_base::register_pending_error(zview err, sl loc) noexcept
{
  if (std::empty(m_pending_error) and not std::empty(err))
  {
    try
    {
      m_pending_error = err;
    }
    catch (std::exception const &e)
    {
      try
      {
        [[unlikely]] process_notice(
          std::format("{} UNABLE TO PROCESS ERROR\n", pqxx::source_loc(loc)));
        // TODO: Make at least an attempt to append a newline.
        process_notice(e.what());
        process_notice("ERROR WAS:\n");
        process_notice(err);
      }
      catch (...)
      {}
    }
  }
}


void pqxx::transaction_base::register_pending_error(
  std::string &&err, sl loc) noexcept
{
  if (std::empty(m_pending_error) and not std::empty(err))
  {
    try
    {
      m_pending_error = std::move(err);
    }
    catch (std::exception const &e)
    {
      try
      {
        process_notice(
          std::format("{} UNABLE TO PROCESS ERROR\n", pqxx::source_loc(loc)));
        // TODO: Make at least an attempt to append a newline.
        process_notice(e.what());
        process_notice("ERROR WAS:\n");
        process_notice(err);
      }
      catch (...)
      {}
    }
  }
}


void pqxx::transaction_base::check_pending_error()
{
  if (not std::empty(m_pending_error))
  {
    // TODO: Store exceptions, or message + source_location.
    std::string err;
    err.swap(m_pending_error);
    throw failure{err};
  }
}


std::string pqxx::transaction_base::description() const
{
  return internal::describe_object("transaction", name());
}


void pqxx::transaction_focus::register_me()
{
  pqxx::internal::gate::transaction_transaction_focus{*m_trans}.register_focus(
    this);
  m_registered = true;
}


void pqxx::transaction_focus::unregister_me() noexcept
{
  pqxx::internal::gate::transaction_transaction_focus{*m_trans}
    .unregister_focus(this);
  m_registered = false;
}


void pqxx::transaction_focus::reg_pending_error(
  std::string const &err, sl loc) noexcept
{
  pqxx::internal::gate::transaction_transaction_focus{*m_trans}
    .register_pending_error(err, loc);
}
