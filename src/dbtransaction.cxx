/*-------------------------------------------------------------------------
 *
 *   FILE
 *	dbtransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::dbtransaction class.
 *   pqxx::dbtransaction represents a real backend transaction
 *
 * Copyright (c) 2004-2008, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


namespace
{
string generate_set_transaction(
	pqxx::readwrite_policy rw,
	const string &IsolationString=string())
{
  string args;

  if (!IsolationString.empty())
    if (IsolationString != pqxx::isolation_traits<pqxx::read_committed>::name())
      args += " ISOLATION LEVEL " + IsolationString;

  if (rw != pqxx::read_write)
    args += " READ ONLY";

  return args.empty() ?
	pqxx::internal::sql_begin_work :
	(string(pqxx::internal::sql_begin_work) + "; SET TRANSACTION" + args);
}
}


pqxx::dbtransaction::dbtransaction(
	connection_base &C,
	const PGSTD::string &IsolationString,
	readwrite_policy rw) :
  namedclass("dbtransaction"),
  transaction_base(C),
  m_StartCmd(generate_set_transaction(rw, IsolationString))
{
}


pqxx::dbtransaction::dbtransaction(
	connection_base &C,
	bool direct,
	readwrite_policy rw) :
  namedclass("dbtransaction"),
  transaction_base(C, direct),
  m_StartCmd(generate_set_transaction(rw))
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


string pqxx::dbtransaction::fullname(const PGSTD::string &ttype,
	const PGSTD::string &isolation)
{
  return ttype + "<" + isolation + ">";
}

