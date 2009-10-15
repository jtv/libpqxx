/*-------------------------------------------------------------------------
 *
 *   FILE
 *	prepared_statement.cxx
 *
 *   DESCRIPTION
 *      Helper classes for defining and executing prepared statements
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006-2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
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

#include "pqxx/internal/gates/connection-prepare-declaration.hxx"
#include "pqxx/internal/gates/connection-prepare-invocation.hxx"


using namespace PGSTD;
using namespace pqxx;
using namespace pqxx::internal;


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
  gate::connection_prepare_declaration(m_home).prepare_param_declare(
	m_statement,
	sqltype,
	treatment);
  return *this;
}


const pqxx::prepare::declaration &
pqxx::prepare::declaration::etc(param_treatment treatment) const
{
  gate::connection_prepare_declaration(m_home).prepare_param_declare_varargs(
	m_statement,
	treatment);
  return *this;
}


pqxx::prepare::invocation::invocation(transaction_base &home,
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
  const int elts = marshall(ptrs, lens);

  return gate::connection_prepare_invocation(m_home.conn()).prepared_exec(
	m_statement,
	ptrs.get(),
	lens.get(),
	elts);
}


bool pqxx::prepare::invocation::exists() const
{
  return gate::connection_prepare_invocation(m_home.conn()).prepared_exists(
	m_statement);
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
  complete(false),
  varargs(false)
{
}


pqxx::prepare::internal::prepared_def::prepared_def(const PGSTD::string &def) :
  definition(def),
  parameters(),
  registered(false),
  complete(false),
  varargs(false)
{
}


void pqxx::prepare::internal::prepared_def::addparam(
	const PGSTD::string &sqltype,
	param_treatment treatment)
{
  parameters.push_back(param(sqltype,treatment));
}



