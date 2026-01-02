/** Implementation of the pqxx::subtransaction class.
 *
 * pqxx::transaction is a nested transaction, i.e. one within a transaction
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <memory>
#include <stdexcept>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/connection.hxx"
#include "pqxx/subtransaction.hxx"

#include "pqxx/internal/header-post.hxx"


using namespace std::literals;


pqxx::subtransaction::subtransaction(
  dbtransaction &t, std::string_view tname, sl loc) :
        transaction_focus{t, "subtransaction"sv, t.conn().adorn_name(tname)},
        // We can't initialise the rollback command here, because we don't yet
        // have a full object to implement quoted_name().
        dbtransaction{t.conn(), tname, std::shared_ptr<std::string>{}, loc}
{
  set_rollback_cmd(std::make_shared<std::string>(
    std::format("ROLLBACK TO SAVEPOINT {}", quoted_name())));
  direct_exec(
    std::make_shared<std::string>(std::format("SAVEPOINT {}", quoted_name())),
    loc);
}


namespace
{
using dbtransaction_ref = pqxx::dbtransaction &;
} // namespace


pqxx::subtransaction::subtransaction(
  subtransaction &t, std::string_view tname, sl loc) :
        subtransaction(dbtransaction_ref(t), tname, loc)
{}


pqxx::subtransaction::~subtransaction() noexcept
{
  close(created_loc());
}


void pqxx::subtransaction::do_commit(sl loc)
{
  direct_exec(
    std::make_shared<std::string>(
      std::format("RELEASE SAVEPOINT {}", quoted_name())),
    loc);
}
