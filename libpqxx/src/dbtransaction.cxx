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
#include "pqxx/compiler.h"

#include "pqxx/dbtransaction"

using namespace std;


pqxx::dbtransaction::dbtransaction(connection_base &C,
    const string &IsolationString,
    const string &NName,
    const string &CName) :
  transaction_base(C, NName, CName),
  m_StartCmd()
{
  if (IsolationString != isolation_traits<read_committed>::name())
    m_StartCmd = "SET TRANSACTION ISOLATION LEVEL " + IsolationString;
}


pqxx::dbtransaction::~dbtransaction()
{
}


void pqxx::dbtransaction::start_backend_transaction()
{
  DirectExec("BEGIN", 2);
  // TODO: Can't we pipeline this to eliminate roundtrip time?
  if (!m_StartCmd.empty()) DirectExec(m_StartCmd.c_str());
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

