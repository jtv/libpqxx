/** Implementation of the pqxx::transaction class.
 *
 * pqxx::transaction represents a regular database transaction.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <stdexcept>

#include "pqxx/connection"
#include "pqxx/result"
#include "pqxx/transaction"


pqxx::internal::basic_transaction::basic_transaction(
  connection &c, char const begin_command[]) :
        namedclass{"transaction"}, dbtransaction(c)
{
  register_transaction();
  direct_exec(begin_command);
}


void pqxx::internal::basic_transaction::do_commit()
{
  static auto const commit{std::make_shared<std::string>("COMMIT")};
  try
  {
    direct_exec(commit);
  }
  catch (statement_completion_unknown const &e)
  {
    // Outcome of "commit" is unknown.  This is a disaster: we don't know the
    // resulting state of the database.
    process_notice(e.what() + std::string{"\n"});
    auto const msg{
      "WARNING: Commit of transaction '" + name() +
      "' is unknown. "
      "There is no way to tell whether the transaction succeeded "
      "or was aborted except to check manually."};
    process_notice(msg + "\n");
    throw in_doubt_error{msg};
  }
  catch (std::exception const &e)
  {
    if (not conn().is_open())
    {
      // We've lost the connection while committing.  There is just no way of
      // telling what happened on the other end.  >8-O
      process_notice(e.what() + std::string{"\n"});

      auto const msg{
        "WARNING: Connection lost while committing transaction "
        "'" +
        name() +
        "'. "
        "There is no way to tell whether the transaction succeeded "
        "or was aborted except to check manually."};

      process_notice(msg + "\n");
      throw in_doubt_error{msg};
    }
    else
    {
      // Commit failed--probably due to a constraint violation or something
      // similar.
      throw;
    }
  }
}


void pqxx::internal::basic_transaction::do_abort()
{
  static auto const rollback{std::make_shared<std::string>("ROLLBACK")};
  direct_exec(rollback);
}
