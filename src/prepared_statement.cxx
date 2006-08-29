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
    const PGSTD::string &statement) :
  m_home(home),
  m_statement(statement)
{
}


const pqxx::prepare::declaration &
pqxx::prepare::declaration::operator()(const PGSTD::string &sqltype,
    param_treatment treatment) const
{
  m_home.prepare_param_declare(m_statement, sqltype, treatment);
  return *this;
}


pqxx::prepare::invocation::invocation(transaction_base &home,
    const PGSTD::string &statement) :
  m_home(home),
  m_statement(statement),
  m_values(),
  m_nonnull()
{
}


pqxx::result pqxx::prepare::invocation::exec() const
{
  const size_t elts = m_nonnull.size();
  pqxx::internal::scoped_array<const char *> ptrs(elts+1);
  pqxx::internal::scoped_array<int> lens(elts+1);
  int v = 0;
  for (size_t i = 0; i < elts; ++i)
  {
    if (m_nonnull[i])
    {
      ptrs[i] = m_values[v].c_str();
      lens[i] = m_values[v].size();
      ++v;
    }
    else
    {
      ptrs[i] = 0;
      lens[i] = 0;
    }
  }

  ptrs[elts] = 0;
  return m_home.prepared_exec(m_statement, ptrs.c_ptr(), lens.c_ptr(), elts);
}


pqxx::prepare::invocation &pqxx::prepare::invocation::operator()()
{
  return setparam("", false);
}


pqxx::prepare::invocation &
pqxx::prepare::invocation::setparam(const PGSTD::string &v, bool nonnull)
{
  m_nonnull.push_back(nonnull);
  if (nonnull) try
  {
    m_values.push_back(v);
  }
  catch (const exception &)
  {
    // This probably doesn't help much, but it makes the function exception-safe
    // at very little cost.
    m_nonnull.resize(m_nonnull.size()-1);
    throw;
  }
  return *this;
}


pqxx::prepare::internal::prepared_def::param::param(
	const PGSTD::string &SQLtype,
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


pqxx::prepare::internal::prepared_def::prepared_def(const PGSTD::string &def) :
  definition(def),
  parameters(),
  registered(false),
  complete(false)
{
}


void pqxx::prepare::internal::prepared_def::addparam(
	const PGSTD::string &sqltype,
	param_treatment treatment)
{
  parameters.push_back(param(sqltype,treatment));
}



