/*-------------------------------------------------------------------------
 *
 *   FILE
 *	prepared_statement.cxx
 *
 *   DESCRIPTION
 *      Helper classes for defining and executing prepared statements
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/connection_base"
#include "pqxx/prepared_statement"
#include "pqxx/result"
#include "pqxx/transaction_base"

using namespace PGSTD;
using namespace pqxx;


pqxx::prepare::declaration::declaration(connection_base &home,
    const string &statement) :
  m_home(home),
  m_statement(statement)
{
}


const pqxx::prepare::declaration &
pqxx::prepare::declaration::operator()(const string &sqltype,
    param_treatment treatment) const
{
  m_home.prepare_param_declare(m_statement, sqltype, treatment);
  return *this;
}


pqxx::prepare::invocation::invocation(transaction_base &home,
    const string &statement) :
  m_home(home),
  m_statement(statement),
  m_values(),
  m_ptrs()
{
}


pqxx::result pqxx::prepare::invocation::exec()
{
  m_ptrs.push_back(0);
  return m_home.prepared_exec(m_statement, &m_ptrs[0], m_ptrs.size()-1);
}


pqxx::prepare::invocation &pqxx::prepare::invocation::operator()()
{
  return setparam("", false);
}


pqxx::prepare::invocation &
pqxx::prepare::invocation::setparam(const string &v, bool nonnull)
{
  const char *val = 0;
  if (nonnull)
  {
    m_values.push_back(v);
    val = m_values.back().c_str();
  }
  m_ptrs.push_back(val);
  return *this;
}


pqxx::prepare::internal::prepared_def::param::param(const string &SQLtype,
    param_treatment Treatment) :
  sqltype(SQLtype),
  treatment(Treatment)
{
}


pqxx::prepare::internal::prepared_def::prepared_def() :
  definition(),
  parameters(),
  registered(false),
  complete(false)
{
}


pqxx::prepare::internal::prepared_def::prepared_def(const string &def) :
  definition(def),
  parameters(),
  registered(false),
  complete(false)
{
}


void pqxx::prepare::internal::prepared_def::addparam(const string &sqltype,
    param_treatment treatment)
{
  parameters.push_back(param(sqltype,treatment));
}



