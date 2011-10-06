/*-------------------------------------------------------------------------
 *
 *   FILE
 *	prepared_statement.cxx
 *
 *   DESCRIPTION
 *      Helper classes for defining and executing prepared statements
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


using namespace PGSTD;
using namespace pqxx;
using namespace pqxx::internal;


pqxx::prepare::invocation::invocation(
	transaction_base &home,
	const PGSTD::string &statement) :
  statement_parameters(),
  m_home(home),
  m_statement(statement)
{
}


pqxx::result pqxx::prepare::invocation::exec() const
{
  scoped_array<const char *> ptrs;
  scoped_array<int> lens;
  scoped_array<int> binaries;
  const int elts = marshall(ptrs, lens, binaries);

  return gate::connection_prepare_invocation(m_home.conn()).prepared_exec(
	m_statement,
	ptrs.get(),
	lens.get(),
	binaries.get(),
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


pqxx::prepare::internal::prepared_def::prepared_def(const PGSTD::string &def) :
  definition(def),
  registered(false)
{
}
