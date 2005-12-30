/*-------------------------------------------------------------------------
 *
 *   FILE
 *	dbtransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::dbtransaction class.
 *   pqxx::dbtransaction represents a real backend transaction
 *
 * Copyright (c) 2004-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/dbtransaction"

using namespace PGSTD;


pqxx::dbtransaction::dbtransaction(connection_base &C,
    const PGSTD::string &IsolationString) :
  namedclass("dbtransaction"),
  transaction_base(C),
  m_StartCmd(internal::sql_begin_work)
{
  if (IsolationString != isolation_traits<read_committed>::name())
    m_StartCmd += ";SET TRANSACTION ISOLATION LEVEL " + IsolationString;
}


pqxx::dbtransaction::dbtransaction(connection_base &C, bool direct) :
  namedclass("dbtransaction"),
  transaction_base(C, direct),
  m_StartCmd(internal::sql_begin_work)
{
}


pqxx::dbtransaction::~dbtransaction()
{
}


void pqxx::dbtransaction::do_begin()
{
  DirectExec(m_StartCmd.c_str(), conn().m_reactivation_avoidance.get() ? 0 : 2);
}


pqxx::result pqxx::dbtransaction::do_exec(const char Query[])
{
  try
  {
    return DirectExec(Query);
  }
  catch (const exception &)
  {
    try { abort(); } catch (const exception &) {}
    throw;
  }
}


void pqxx::dbtransaction::do_abort()
{
  reactivation_avoidance_clear();
  DirectExec(internal::sql_rollback_work);
}


string pqxx::dbtransaction::fullname(const string &ttype,
	const string &isolation)
{
  return ttype + "<" + isolation + ">";
}

