/*-------------------------------------------------------------------------
 *
 *   FILE
 *	result.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::result class and support classes.
 *   pqxx::result represents the set of result tuples from a database query
 *
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include "libpq-fe.h"

#include "pqxx/except"
#include "pqxx/field"
#include "pqxx/result"
#include "pqxx/tuple"


using namespace PGSTD;

const string pqxx::result::s_empty_string;


pqxx::internal::result_data::result_data() :
  data(0),
  protocol(0),
  query(),
  encoding_code(0)
{}

pqxx::internal::result_data::result_data(pqxx::internal::pq::PGresult *d,
	int p,
	const PGSTD::string &q,
	int e) :
  data(d),
  protocol(p),
  query(q),
  encoding_code(e)
{}


pqxx::internal::result_data::~result_data() { PQclear(data); }


void pqxx::internal::freemem_result_data(const result_data *d) throw ()
	{ delete d; }


pqxx::result::result(pqxx::internal::pq::PGresult *rhs,
	int protocol,
	const PGSTD::string &Query,
	int encoding_code) :
  super(new internal::result_data(rhs, protocol, Query, encoding_code)),
  m_data(rhs)
{}

bool pqxx::result::operator==(const result &rhs) const throw ()
{
  if (&rhs == this) return true;
  const size_type s(size());
  if (rhs.size() != s) return false;
  for (size_type i=0; i<s; ++i)
    if ((*this)[i] != rhs[i]) return false;
  return true;
}


pqxx::result::const_reverse_iterator pqxx::result::rbegin() const
{
  return const_reverse_iterator(end());
}


pqxx::result::const_reverse_iterator pqxx::result::rend() const
{
  return const_reverse_iterator(begin());
}


pqxx::result::const_iterator pqxx::result::begin() const throw ()
{
  return const_iterator(this, 0);
}


pqxx::result::size_type pqxx::result::size() const throw ()
{
  return m_data ? size_type(PQntuples(m_data)) : 0;
}


bool pqxx::result::empty() const throw ()
{
  return !m_data || !PQntuples(m_data);
}


void pqxx::result::swap(result &rhs) throw ()
{
  super::swap(rhs);
  m_data = (get() ? get()->data : 0);
  rhs.m_data = (rhs.get() ? rhs.get()->data : 0);
}


const pqxx::tuple pqxx::result::at(pqxx::result::size_type i) const
  throw (range_error)
{
  if (i >= size()) throw range_error("Tuple number out of range");
  return operator[](i);
}


void pqxx::result::ThrowSQLError(
	const PGSTD::string &Err,
	const PGSTD::string &Query) const
{
  // Try to establish more precise error type, and throw corresponding exception
  const char *const code = PQresultErrorField(m_data, PG_DIAG_SQLSTATE);
  if (code) switch (code[0])
  {
  case '0':
    switch (code[1])
    {
    case '8':
      throw broken_connection(Err);
    case 'A':
      throw feature_not_supported(Err, Query);
    }
    break;
  case '2':
    switch (code[1])
    {
    case '2':
      throw data_exception(Err, Query);
    case '3':
      if (strcmp(code,"23001")==0) throw restrict_violation(Err, Query);
      if (strcmp(code,"23502")==0) throw not_null_violation(Err, Query);
      if (strcmp(code,"23503")==0) throw foreign_key_violation(Err, Query);
      if (strcmp(code,"23505")==0) throw unique_violation(Err, Query);
      if (strcmp(code,"23514")==0) throw check_violation(Err, Query);
      throw integrity_constraint_violation(Err, Query);
    case '4':
      throw invalid_cursor_state(Err, Query);
    case '6':
      throw invalid_sql_statement_name(Err, Query);
    }
    break;
  case '3':
    switch (code[1])
    {
    case '4':
      throw invalid_cursor_name(Err, Query);
    }
    break;
  case '4':
    switch (code[1])
    {
    case '2':
      if (strcmp(code,"42501")==0) throw insufficient_privilege(Err, Query);
      if (strcmp(code,"42601")==0)
        throw syntax_error(Err, Query, errorposition());
      if (strcmp(code,"42703")==0) throw undefined_column(Err, Query);
      if (strcmp(code,"42883")==0) throw undefined_function(Err, Query);
      if (strcmp(code,"42P01")==0) throw undefined_table(Err, Query);
    }
    break;
  case '5':
    switch (code[1])
    {
    case '3':
      if (strcmp(code,"53100")==0) throw disk_full(Err, Query);
      if (strcmp(code,"53200")==0) throw out_of_memory(Err, Query);
      if (strcmp(code,"53300")==0) throw too_many_connections(Err);
      throw insufficient_resources(Err, Query);
    }
    break;

  case 'P':
    if (strcmp(code, "P0001")==0) throw plpgsql_raise(Err, Query);
    if (strcmp(code, "P0002")==0) throw plpgsql_no_data_found(Err, Query);
    if (strcmp(code, "P0003")==0) throw plpgsql_too_many_rows(Err, Query);
    throw plpgsql_error(Err, Query);
  }
  throw sql_error(Err, Query);
}

void pqxx::result::CheckStatus() const
{
  const string Err = StatusError();
  if (!Err.empty()) ThrowSQLError(Err, query());
}


string pqxx::result::StatusError() const
{
  if (!m_data) throw failure("No result set given");

  string Err;

  switch (PQresultStatus(m_data))
  {
  case PGRES_EMPTY_QUERY: // The string sent to the backend was empty.
  case PGRES_COMMAND_OK: // Successful completion of a command returning no data
  case PGRES_TUPLES_OK: // The query successfully executed
    break;

  case PGRES_COPY_OUT: // Copy Out (from server) data transfer started
  case PGRES_COPY_IN: // Copy In (to server) data transfer started
    break;

  case PGRES_BAD_RESPONSE: // The server's response was not understood
  case PGRES_NONFATAL_ERROR:
  case PGRES_FATAL_ERROR:
    Err = PQresultErrorMessage(m_data);
    break;

  default:
    throw internal_error("pqxx::result: Unrecognized response code " +
		      to_string(int(PQresultStatus(m_data))));
  }
  return Err;
}


const char *pqxx::result::CmdStatus() const throw ()
{
  return PQcmdStatus(m_data);
}


const string &pqxx::result::query() const throw ()
{
  return get() ? get()->query : s_empty_string;
}


pqxx::oid pqxx::result::inserted_oid() const
{
  if (!m_data)
    throw usage_error("Attempt to read oid of inserted row without an INSERT "
	"result");
  return PQoidValue(m_data);
}


pqxx::result::size_type pqxx::result::affected_rows() const
{
  const char *const RowsStr = PQcmdTuples(m_data);
  return RowsStr[0] ? size_type(atoi(RowsStr)) : 0;
}


const char *pqxx::result::GetValue(
	pqxx::result::size_type Row,
	pqxx::tuple::size_type Col) const
{
  return PQgetvalue(m_data, int(Row), int(Col));
}


bool pqxx::result::GetIsNull(
	pqxx::result::size_type Row,
	pqxx::tuple::size_type Col) const
{
  return PQgetisnull(m_data, int(Row), int(Col)) != 0;
}

pqxx::field::size_type pqxx::result::GetLength(
	pqxx::result::size_type Row,
        pqxx::tuple::size_type Col) const throw ()
{
  return field::size_type(PQgetlength(m_data, int(Row), int(Col)));
}


pqxx::oid pqxx::result::column_type(tuple::size_type ColNum) const
{
  const oid T = PQftype(m_data, int(ColNum));
  if (T == oid_none)
    throw argument_error(
	"Attempt to retrieve type of nonexistant column " +
	to_string(ColNum) + " of query result");
  return T;
}


pqxx::oid pqxx::result::column_table(tuple::size_type ColNum) const
{
  const oid T = PQftable(m_data, int(ColNum));

  /* If we get oid_none, it may be because the column is computed, or because we
   * got an invalid row number.
   */
  if (T == oid_none && ColNum >= columns())
    throw argument_error("Attempt to retrieve table ID for column " +
	to_string(ColNum) + " out of " + to_string(columns()));

  return T;
}


pqxx::tuple::size_type pqxx::result::table_column(tuple::size_type ColNum) const
{
  const tuple::size_type n = tuple::size_type(PQftablecol(m_data, int(ColNum)));
  if (n) return n-1;

  // Failed.  Now find out why, so we can throw a sensible exception.
  // Possible reasons:
  // 1. Column out of range
  // 2. Not using protocol 3.0 or better
  // 3. Column not taken directly from a table
  if (ColNum > columns())
    throw range_error("Invalid column index in table_column(): " +
      to_string(ColNum));

  if (!get() || get()->protocol < 3)
    throw feature_not_supported("Backend version does not support querying of "
      "column's original number",
      "[TABLE_COLUMN]");

  throw usage_error("Can't query origin of column " + to_string(ColNum) + ": "
	"not derived from table column");
}

int pqxx::result::errorposition() const throw ()
{
  int pos = -1;
  if (m_data)
  {
    const char *p = PQresultErrorField(m_data, PG_DIAG_STATEMENT_POSITION);
    if (p) from_string(p, pos);
  }
  return pos;
}


const char *pqxx::result::column_name(pqxx::tuple::size_type Number) const
{
  const char *const N = PQfname(m_data, int(Number));
  if (!N)
    throw range_error("Invalid column number: " + to_string(Number));

  return N;
}


pqxx::tuple::size_type pqxx::result::columns() const throw ()
{
  return m_data ? tuple::size_type(PQnfields(m_data)) : 0;
}


// const_result_iterator

pqxx::const_result_iterator pqxx::const_result_iterator::operator++(int)
{
  const_result_iterator old(*this);
  m_Index++;
  return old;
}


pqxx::const_result_iterator pqxx::const_result_iterator::operator--(int)
{
  const_result_iterator old(*this);
  m_Index--;
  return old;
}


pqxx::result::const_iterator
pqxx::result::const_reverse_iterator::base() const throw ()
{
  iterator_type tmp(*this);
  return ++tmp;
}


pqxx::const_reverse_result_iterator
pqxx::const_reverse_result_iterator::operator++(int)
{
  const_reverse_result_iterator tmp(*this);
  iterator_type::operator--();
  return tmp;
}


pqxx::const_reverse_result_iterator
pqxx::const_reverse_result_iterator::operator--(int)
{
  const_reverse_result_iterator tmp(*this);
  iterator_type::operator++();
  return tmp;
}
