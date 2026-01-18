/** Implementation of the pqxx::transaction class.
 *
 * pqxx::transaction represents a regular database transaction.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <stdexcept>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/connection.hxx"
#include "pqxx/result.hxx"
#include "pqxx/transaction.hxx"

#include "pqxx/internal/header-post.hxx"


pqxx::internal::basic_transaction::basic_transaction(
  connection &cx, zview begin_command, std::string_view tname, sl loc) :
        dbtransaction(cx, tname, loc)
{
  register_transaction();
  direct_exec(begin_command, loc);
}


pqxx::internal::basic_transaction::basic_transaction(
  connection &cx, zview begin_command, std::string &&tname, sl loc) :
        dbtransaction(cx, std::move(tname), loc)
{
  register_transaction();
  direct_exec(begin_command, loc);
}


pqxx::internal::basic_transaction::basic_transaction(
  connection &cx, zview begin_command, sl loc) :
        dbtransaction(cx, loc)
{
  register_transaction();
  direct_exec(begin_command, loc);
}


// This should stop the compiler from generating the same vtables and
// destructor in multiple translation units.  More importantly, if we don't do
// this, the sanitisers in g++ 7 and clang++ 6 complain about pointers to
// dbtransaction actually pointing to basic_transaction.  Which is odd, in that
// any basic_transaction pointer should also be a dbtransaction pointer.  But,
// apparently the vtable isn't the right one.
pqxx::internal::basic_transaction::~basic_transaction() noexcept = default;


void pqxx::internal::basic_transaction::do_commit(sl loc)
{
  static auto const commit_q{std::make_shared<std::string>("COMMIT"sv)};
  try
  {
    direct_exec(commit_q, loc);
  }
  catch (statement_completion_unknown const &e)
  {
    // Outcome of "commit" is unknown.  This is a disaster: we don't know the
    // resulting state of the database.
    process_notice(std::format("{}\n", e.what()));

    std::string msg{std::format(
      "WARNING: {}: Commit status of transaction '{}' is unknown. "
      "There is no way to tell whether the transaction succeeded "
      "or was aborted except to check manually.\n",
      pqxx::source_loc(loc), name())};
    process_notice(msg);
    // Strip newline.  It was only needed for process_notice().
    msg.pop_back();
    throw in_doubt_error{msg, e.location()};
  }
  catch (std::exception const &e)
  {
    if (not conn().is_open())
    {
      // We've lost the connection while committing.  There is just no way of
      // telling what happened on the other end.  >8-O
      process_notice(std::format("{}\n", e.what()));

      auto msg{std::format(
        "WARNING: {}: Connection lost while committing transaction '{}'.  "
        "There is no way to tell whether the transaction succeeded or was "
        "aborted except to check manually.\n",
        pqxx::source_loc(loc), name())};
      process_notice(msg);
      // Strip newline.  It was only needed for process_notice().
      msg.pop_back();
      throw in_doubt_error{msg, loc};
    }
    else
    {
      // Commit failed--probably due to a constraint violation or something
      // similar.
      throw;
    }
  }
}
