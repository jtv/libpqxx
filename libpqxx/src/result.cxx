/*-------------------------------------------------------------------------
 *
 *   FILE
 *	result.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::result class and support classes.
 *   pqxx::result represents the set of result tuples from a database query
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <stdexcept>

#include "pqxx/except"
#include "pqxx/result"


using namespace PGSTD;
using namespace pqxx::internal::pq;


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


void pqxx::result::swap(result &rhs) throw ()
{
  super::swap(rhs);
}


const pqxx::result::tuple pqxx::result::at(pqxx::result::size_type i) const
  throw (out_of_range)
{
  if (i >= size())
    throw out_of_range("Tuple number out of range");

  return operator[](i);
}


void pqxx::result::CheckStatus(const string &Query) const
{
  const string Err = StatusError();
  if (!Err.empty()) throw sql_error(Err, Query);
}


void pqxx::result::CheckStatus(const char Query[]) const
{
  const string Err = StatusError();
  if (!Err.empty()) throw sql_error(Err, string(Query ? Query : ""));
}


string pqxx::result::StatusError() const
{
  if (!c_ptr())
    throw runtime_error("No result");

  string Err;

  switch (PQresultStatus(c_ptr()))
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
    Err = PQresultErrorMessage(c_ptr());
    break;

  default:
    throw logic_error("libpqxx internal error: "
		      "pqxx::result: Unrecognized response code " +
		      to_string(int(PQresultStatus(c_ptr()))));
  }
  return Err;
}


pqxx::result::size_type pqxx::result::affected_rows() const
{
  const char *const RowsStr = PQcmdTuples(c_ptr());
  return RowsStr[0] ? atoi(RowsStr) : 0;
}


const char *pqxx::result::GetValue(pqxx::result::size_type Row, 
		                 pqxx::result::tuple::size_type Col) const
{
  return PQgetvalue(c_ptr(), Row, Col);
}


bool pqxx::result::GetIsNull(pqxx::result::size_type Row,
		           pqxx::result::tuple::size_type Col) const
{
  return PQgetisnull(c_ptr(), Row, Col) != 0;
}

pqxx::result::field::size_type 
pqxx::result::GetLength(pqxx::result::size_type Row,
                        pqxx::result::tuple::size_type Col) const
{
  return PQgetlength(c_ptr(), Row, Col);
}


int pqxx::result::errorposition() const throw ()
{
  int pos = -1;
#if defined(PQXX_HAVE_PQRESULTERRORFIELD)
  if (c_ptr())
  {
    const char *p = PQresultErrorField(c_ptr(), PG_DIAG_STATEMENT_POSITION);
    if (p) from_string(p, pos);
  }
#endif // PQXX_HAVE_PQRESULTERRORFIELD
  return pos;
}


// tuple

pqxx::result::field pqxx::result::tuple::operator[](const char f[]) const
{
  return field(*this, m_Home->column_number(f));
}


pqxx::result::field pqxx::result::tuple::at(const char f[]) const
{
  const int fnum = m_Home->column_number(f);
  if (fnum == -1)
    throw invalid_argument(string("Unknown field '") + f + "'");

  return field(*this, fnum);
}


pqxx::result::field 
pqxx::result::tuple::at(pqxx::result::tuple::size_type i) const throw (out_of_range)
{
  if (i >= size())
    throw out_of_range("Invalid field number");

  return operator[](i);
}


const char *
pqxx::result::column_name(pqxx::result::tuple::size_type Number) const
{
  const char *const N = PQfname(c_ptr(), Number);
  if (!N) 
    throw out_of_range("Invalid column number: " + to_string(Number));

  return N;
}


pqxx::result::tuple::size_type
pqxx::result::column_number(const char ColName[]) const
{
  const int N = PQfnumber(c_ptr(), ColName);
  if (N == -1)
    throw invalid_argument("Unknown column name: '" + string(ColName) + "'");

  return tuple::size_type(N);
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

