/*-------------------------------------------------------------------------
 *
 *   FILE
 *	except.cxx
 *
 *   DESCRIPTION
 *      implementation of libpqxx exception classes
 *
 * Copyright (c) 2005-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/except"


pqxx::pqxx_exception::~pqxx_exception() PQXX_NOEXCEPT
{
}


pqxx::failure::failure(const std::string &whatarg) :
  pqxx_exception(),
  std::runtime_error(whatarg)
{
}

pqxx::broken_connection::broken_connection() :
  failure("Connection to database failed")
{
}

pqxx::broken_connection::broken_connection(const std::string &whatarg) :
  failure(whatarg)
{
}

pqxx::sql_error::sql_error() :
  failure("Failed query"),
  m_Q()
{
}

pqxx::sql_error::sql_error(const std::string &whatarg) :
  failure(whatarg),
  m_Q()
{
}

pqxx::sql_error::sql_error(const std::string &whatarg,
	const std::string &Q) :
  failure(whatarg),
  m_Q(Q)
{
}

pqxx::sql_error::~sql_error() PQXX_NOEXCEPT
{
}


PQXX_CONST const std::string &pqxx::sql_error::query() const PQXX_NOEXCEPT
{
  return m_Q;
}


pqxx::in_doubt_error::in_doubt_error(const std::string &whatarg) :
  failure(whatarg)
{
}


pqxx::internal_error::internal_error(const std::string &whatarg) :
  logic_error("libpqxx internal error: " + whatarg)
{
}


pqxx::usage_error::usage_error(const std::string &whatarg) :
  logic_error(whatarg)
{
}


pqxx::argument_error::argument_error(const std::string &whatarg) :
  invalid_argument(whatarg)
{
}


pqxx::conversion_error::conversion_error(const std::string &whatarg) :
  domain_error(whatarg)
{
}


pqxx::range_error::range_error(const std::string &whatarg) :
  out_of_range(whatarg)
{
}

