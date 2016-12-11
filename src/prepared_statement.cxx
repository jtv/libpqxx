/*-------------------------------------------------------------------------
 *
 *   FILE
 *	prepared_statement.cxx
 *
 *   DESCRIPTION
 *      Helper classes for defining and executing prepared statements
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006-2016, Jeroen T. Vermeulen <jtv@xs4all.nl>
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

#include "pqxx/internal/gates/connection-prepare-invocation.hxx"


using namespace pqxx;
using namespace pqxx::internal;


pqxx::prepare::invocation::invocation(
	transaction_base &home,
	const std::string &statement) :
  statement_parameters(),
  m_home(home),
  m_statement(statement)
{
}


pqxx::result pqxx::prepare::invocation::exec() const
{
  std::vector<const char *> ptrs;
  std::vector<int> lens;
  std::vector<int> binaries;
  const int elts = marshall(ptrs, lens, binaries);

  return gate::connection_prepare_invocation(m_home.conn()).prepared_exec(
	m_statement,
	&ptrs[0],
	&lens[0],
	&binaries[0],
	elts);
}


bool pqxx::prepare::invocation::exists() const
{
  return gate::connection_prepare_invocation(m_home.conn()).prepared_exists(
	m_statement);
}


pqxx::prepare::internal::prepared_def::prepared_def() :
  definition(),
  registered(false)
{
}


pqxx::prepare::internal::prepared_def::prepared_def(const std::string &def) :
  definition(def),
  registered(false)
{
}
