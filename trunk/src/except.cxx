/*-------------------------------------------------------------------------
 *
 *   FILE
 *	except.cxx
 *
 *   DESCRIPTION
 *      implementation of libpqxx exception classes
 *
 * Copyright (c) 2005-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/except"

using namespace PGSTD;

pqxx::broken_connection::broken_connection() :
  runtime_error("Connection to database failed")
{
}

pqxx::broken_connection::broken_connection(const string &whatarg) :
  runtime_error(whatarg)
{
}

pqxx::sql_error::sql_error() :
  runtime_error("Failed query"),
  m_Q()
{
}

pqxx::sql_error::sql_error(const string &whatarg) :
  runtime_error(whatarg),
  m_Q()
{
}

pqxx::sql_error::sql_error(const string &whatarg, const string &Q) :
  runtime_error(whatarg),
  m_Q(Q)
{
}

pqxx::sql_error::~sql_error() throw ()
{
}


const string &pqxx::sql_error::query() const throw () { return m_Q; }


pqxx::in_doubt_error::in_doubt_error(const string &whatarg) :
  runtime_error(whatarg)
{
}


pqxx::internal_error::internal_error(const string &whatarg) :
  logic_error("libpqxx internal error: " + whatarg)
{
}

