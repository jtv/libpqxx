/** Implementation of the pqxx::subtransaction class.
 *
 * pqxx::transaction is a nested transaction, i.e. one within a transaction
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx-source.hxx"

#include <stdexcept>

#include "pqxx/connection"
#include "pqxx/subtransaction"


pqxx::subtransaction::subtransaction(
	dbtransaction &T,
	const std::string &Name) :
  namedclass{"subtransaction", T.conn().adorn_name(Name)},
  transactionfocus{T},
  dbtransaction(T.conn())
{
  direct_exec(("SAVEPOINT " + quoted_name()).c_str());
}


namespace
{
using dbtransaction_ref = pqxx::dbtransaction &;
}


pqxx::subtransaction::subtransaction(
	subtransaction &T,
	const std::string &Name) :
  subtransaction(dbtransaction_ref(T), Name)
{
}


void pqxx::subtransaction::do_commit()
{
  direct_exec(("RELEASE SAVEPOINT " + quoted_name()).c_str());
}


void pqxx::subtransaction::do_abort()
{
  direct_exec(("ROLLBACK TO SAVEPOINT " + quoted_name()).c_str());
}
