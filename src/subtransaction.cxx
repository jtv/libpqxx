/** Implementation of the pqxx::subtransaction class.
 *
 * pqxx::transaction is a nested transaction, i.e. one within a transaction
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <memory>
#include <stdexcept>

#include "pqxx/connection"
#include "pqxx/subtransaction"

#include "pqxx/internal/concat.hxx"


namespace
{
using namespace std::literals;
constexpr std::string_view class_name{"subtransaction"sv};
} // namespace


pqxx::subtransaction::subtransaction(
  dbtransaction &t, std::string_view tname) :
        transactionfocus{t, class_name, t.conn().adorn_name(tname)},
        // Initialisation order is crucial here.  We don't get a working
        // quoted_name() until our transactionfocus base class object is
        // complete.
        dbtransaction{
            t.conn(),
            tname,
            std::make_shared<std::string>(internal::concat("ROLLBACK TO SAVEPOINT ", quoted_name()))}
{
  direct_exec(std::make_shared<std::string>(
    internal::concat("SAVEPOINT ", quoted_name())));
}


namespace
{
using dbtransaction_ref = pqxx::dbtransaction &;
}


pqxx::subtransaction::subtransaction(
  subtransaction &t, std::string_view tname) :
        subtransaction(dbtransaction_ref(t), tname)
{}


void pqxx::subtransaction::do_commit()
{
  direct_exec(std::make_shared<std::string>(
    internal::concat("RELEASE SAVEPOINT ", quoted_name())));
}
