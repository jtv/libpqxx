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
#include "pqxx/result"


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


bool pqxx::result::tuple::operator==(const tuple &rhs) const throw ()
{
  if (&rhs == this) return true;
  const size_type s(size());
  if (rhs.size() != s) return false;
  // TODO: Depends on how null is handled!
  for (size_type i=0; i<s; ++i) if ((*this)[i] != rhs[i]) return false;
  return true;
}


void pqxx::result::tuple::swap(tuple &rhs) throw ()
{
  const result *const h(m_Home);
  const result::size_type i(m_Index);
  const size_type b(m_Begin);
  const size_type e(m_End);
  m_Home = rhs.m_Home;
  m_Index = rhs.m_Index;
  m_Begin = rhs.m_Begin;
  m_End = rhs.m_End;
  rhs.m_Home = h;
  rhs.m_Index = i;
  rhs.m_Begin = b;
  rhs.m_End = e;
}


bool pqxx::result::field::operator==(const field &rhs) const
{
  if (is_null() != rhs.is_null()) return false;
  // TODO: Verify null handling decision
  const size_type s = size();
  if (s != rhs.size()) return false;
  const char *const l(c_str()), *const r(rhs.c_str());
  for (size_type i = 0; i < s; ++i) if (l[i] != r[i]) return false;
  return true;
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


const pqxx::result::tuple pqxx::result::at(pqxx::result::size_type i) const
  throw (range_error)
{
  if (i >= size()) throw range_error("Tuple number out of range");
  return operator[](i);
}


void pqxx::result::ThrowSQLError(
	const PGSTD::string &Err,
	const PGSTD::string &Query) const
{
#if defined(PQXX_HAVE_PQRESULTERRORFIELD)
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
#endif
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


const char *pqxx::result::GetValue(pqxx::result::size_type Row,
		                 pqxx::result::tuple::size_type Col) const
{
  return PQgetvalue(m_data, int(Row), int(Col));
}


bool pqxx::result::GetIsNull(pqxx::result::size_type Row,
		           pqxx::result::tuple::size_type Col) const
{
  return PQgetisnull(m_data, int(Row), int(Col)) != 0;
}

pqxx::result::field::size_type
pqxx::result::GetLength(pqxx::result::size_type Row,
                        pqxx::result::tuple::size_type Col) const throw ()
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


#ifdef PQXX_HAVE_PQFTABLE
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
#endif


#ifdef PQXX_HAVE_PQFTABLECOL
pqxx::result::tuple::size_type
pqxx::result::table_column(tuple::size_type ColNum) const
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
#endif

int pqxx::result::errorposition() const throw ()
{
  int pos = -1;
#if defined(PQXX_HAVE_PQRESULTERRORFIELD)
  if (m_data)
  {
    const char *p = PQresultErrorField(m_data, PG_DIAG_STATEMENT_POSITION);
    if (p) from_string(p, pos);
  }
#endif // PQXX_HAVE_PQRESULTERRORFIELD
  return pos;
}


// tuple

pqxx::result::field pqxx::result::tuple::at(const char f[]) const
{
  return field(*this, m_Begin + column_number(f));
}


pqxx::result::field
pqxx::result::tuple::at(pqxx::result::tuple::size_type i) const
  throw (range_error)
{
  if (i >= size())
    throw range_error("Invalid field number");

  return operator[](i);
}


const char *
pqxx::result::column_name(pqxx::result::tuple::size_type Number) const
{
  const char *const N = PQfname(m_data, int(Number));
  if (!N)
    throw range_error("Invalid column number: " + to_string(Number));

  return N;
}


pqxx::result::tuple::size_type pqxx::result::columns() const throw ()
{
  return m_data ? tuple::size_type(PQnfields(m_data)) : 0;
}


pqxx::result::tuple::size_type
pqxx::result::tuple::column_number(const char ColName[]) const
{
  const size_type n = m_Home->column_number(ColName);
  if (n >= m_End)
    return result().column_number(ColName);
  if (n >= m_Begin)
    return n - m_Begin;

  const char *const AdaptedColName = m_Home->column_name(n);
  for (size_type i = m_Begin; i < m_End; ++i)
    if (strcmp(AdaptedColName, m_Home->column_name(i)) == 0)
      return i - m_Begin;

  return result().column_number(ColName);
}


pqxx::result::tuple::size_type
pqxx::result::column_number(const char ColName[]) const
{
  const int N = PQfnumber(m_data, ColName);
  // TODO: Should this be an out_of_range?
  if (N == -1)
    throw argument_error("Unknown column name: '" + string(ColName) + "'");

  return tuple::size_type(N);
}


pqxx::result::tuple
pqxx::result::tuple::slice(size_type Begin, size_type End) const
{
  if (Begin > End || End > size())
    throw range_error("Invalid field range");

  tuple result(*this);
  result.m_Begin = m_Begin + Begin;
  result.m_End = m_Begin + End;
  return result;
}


bool pqxx::result::tuple::empty() const throw ()
{
  return m_Begin == m_End;
}


// const_iterator

pqxx::result::const_iterator
pqxx::result::const_iterator::operator++(int)
{
  const_iterator old(*this);
  m_Index++;
  return old;
}


pqxx::result::const_iterator
pqxx::result::const_iterator::operator--(int)
{
  const_iterator old(*this);
  m_Index--;
  return old;
}



// const_fielditerator

pqxx::result::const_fielditerator
pqxx::result::const_fielditerator::operator++(int)
{
  const_fielditerator old(*this);
  m_col++;
  return old;
}


pqxx::result::const_fielditerator
pqxx::result::const_fielditerator::operator--(int)
{
  const_fielditerator old(*this);
  m_col--;
  return old;
}


pqxx::result::const_iterator
pqxx::result::const_reverse_iterator::base() const throw ()
{
  iterator_type tmp(*this);
  return ++tmp;
}


pqxx::result::const_reverse_iterator
pqxx::result::const_reverse_iterator::operator++(int)
{
  const_reverse_iterator tmp(*this);
  iterator_type::operator--();
  return tmp;
}


pqxx::result::const_reverse_iterator
pqxx::result::const_reverse_iterator::operator--(int)
{
  const_reverse_iterator tmp(*this);
  iterator_type::operator++();
  return tmp;
}


pqxx::result::const_fielditerator
pqxx::result::const_reverse_fielditerator::base() const throw ()
{
  iterator_type tmp(*this);
  return ++tmp;
}


pqxx::result::const_reverse_fielditerator
pqxx::result::const_reverse_fielditerator::operator++(int)
{
  const_reverse_fielditerator tmp(*this);
  operator++();
  return tmp;
}


pqxx::result::const_reverse_fielditerator
pqxx::result::const_reverse_fielditerator::operator--(int)
{
  const_reverse_fielditerator tmp(*this);
  operator--();
  return tmp;
}


