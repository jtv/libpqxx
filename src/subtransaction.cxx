/*-------------------------------------------------------------------------
 *
 *   FILE
 *	subtransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::subtransaction class.
 *   pqxx::transaction is a nested transaction, i.e. one within a transaction
 *
 * Copyright (c) 2005-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
#include "pqxx/subtransaction"


using namespace PGSTD;


pqxx::subtransaction::subtransaction(dbtransaction &T,
    const string &Name) :
  namedclass("subtransaction", T.conn().adorn_name(Name)),
  transactionfocus(T),
  dbtransaction(T.conn(), false),
  m_parent(T)
{
  check_backendsupport();
}


void pqxx::subtransaction::do_begin()
{
  try
  {
    DirectExec(("SAVEPOINT \"" + name() + "\"").c_str());
  }
  catch (const sql_error &)
  {
    check_backendsupport();
    throw;
  }
}


void pqxx::subtransaction::do_commit()
{
  const int ra = m_reactivation_avoidance.get();
  m_reactivation_avoidance.clear();
  DirectExec(("RELEASE SAVEPOINT \"" + name() + "\"").c_str());
  m_parent.m_reactivation_avoidance.add(ra);
}


void pqxx::subtransaction::do_abort()
{
  DirectExec(("ROLLBACK TO SAVEPOINT \"" + name() + "\"").c_str());
}


void pqxx::subtransaction::check_backendsupport() const
{
#if defined(PQXX_HAVE_PQSERVERVERSION)
  if (!T.conn().supports(connection_base::cap_nested_transactions))
    throw runtime_error("Backend version does not support nested transactions");
#endif
}

