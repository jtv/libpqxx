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


pqxx::subtransaction::subtransaction(
  dbtransaction &t, std::string const &Name) :
        namedclass{"subtransaction", t.conn().adorn_name(Name)},
        transactionfocus{t},
        dbtransaction(t.conn())
{
  direct_exec(std::make_shared<std::string>("SAVEPOINT " + quoted_name()));
}


namespace
{
using dbtransaction_ref = pqxx::dbtransaction &;
}


pqxx::subtransaction::subtransaction(
  subtransaction &t, std::string const &name) :
        subtransaction(dbtransaction_ref(t), name)
{}


void pqxx::subtransaction::do_commit()
{
  direct_exec(
    std::make_shared<std::string>("RELEASE SAVEPOINT " + quoted_name()));
}


void pqxx::subtransaction::do_abort()
{
  direct_exec(
    std::make_shared<std::string>("ROLLBACK TO SAVEPOINT " + quoted_name()));
}
