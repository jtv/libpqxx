/*-------------------------------------------------------------------------
 *
 *   FILE
 *	dbtransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::dbtransaction class.
 *   pqxx::dbtransaction represents a real backend transaction
 *
 * Copyright (c) 2004-2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/dbtransaction"

#include "pqxx/internal/gates/connection-dbtransaction.hxx"

using namespace PGSTD;
using namespace pqxx::internal;


namespace
{
string generate_set_transaction(
	pqxx::connection_base &C,
	pqxx::readwrite_policy rw,
	const string &IsolationString=string())
{
  string args;

  if (!IsolationString.empty())
    if (IsolationString != pqxx::isolation_traits<pqxx::read_committed>::name())
      args += " ISOLATION LEVEL " + IsolationString;

  if (rw != pqxx::read_write &&
      C.supports(pqxx::connection_base::cap_read_only_transactions))
    args += " READ ONLY";

  return args.empty() ?
	pqxx::internal::sql_begin_work :
	(string(pqxx::internal::sql_begin_work) + "; SET TRANSACTION" + args);
}
} // namespace


pqxx::dbtransaction::dbtransaction(
	connection_base &C,
	const PGSTD::string &IsolationString,
	readwrite_policy rw) :
  namedclass("dbtransaction"),
  transaction_base(C),
  m_StartCmd(generate_set_transaction(C, rw, IsolationString))
{
}


pqxx::dbtransaction::dbtransaction(
	connection_base &C,
	bool direct,
	readwrite_policy rw) :
  namedclass("dbtransaction"),
  transaction_base(C, direct),
  m_StartCmd(generate_set_transaction(C, rw))
{
}


pqxx::dbtransaction::~dbtransaction()
{
}


void pqxx::dbtransaction::do_begin()
{
  const gate::connection_dbtransaction gate(conn());
  const int avoidance_counter = gate.get_reactivation_avoidance_count();
  DirectExec(m_StartCmd.c_str(), avoidance_counter ? 0 : 2);
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

