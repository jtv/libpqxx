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


pqxx::result &pqxx::result::operator=(const pqxx::result &Other) throw ()
{
  if (&Other != this)
  {
    LoseRef();
    MakeRef(Other);
  }

  return *this;
}


pqxx::result &pqxx::result::operator=(internal::pq::PGresult *Other) throw ()
{
  if (Other != m_Result)
  {
    LoseRef();
    MakeRef(Other);
  }
  return *this;
}


bool pqxx::result::operator==(const result &rhs) const throw ()
{
  if (&rhs == this) return true;
  const size_type s(size());
  if (rhs.size() != s) return false;
  for (size_type i=0; i<s; ++i)
    if ((*this)[i] != rhs[i]) return false;
  return true;
}


bool pqxx::result::operator<(const result &rhs) const throw ()
{
  if (&rhs == this) return false;
  const size_type s = size();
  // TODO: Is this correct?
  if (s > rhs.size()) return false;
  for (size_type i=0; i<s; ++i) if (!((*this)[i] < rhs[i])) return false;
  return true;
}


bool pqxx::result::tuple::operator==(const tuple &rhs) const throw ()
{
  if (&rhs == this) return true;
  const size_type s(size());
  if (rhs.size() != s) return false;
  for (size_type i=0; i<s; ++i)
  {
    const field l((*this)[i]), r(rhs[i]);
    if (l.size() != r.size() || l.is_null() != r.is_null()) return false;
    if (!l.is_null() && (l.as<string>() != r.as<string>())) return false;
  }
  return true;
}


bool pqxx::result::tuple::operator<(const tuple &rhs) const throw ()
{
  if (&rhs == this) return false;
  const size_type s = size();
  // TODO: Is this correct?
  if (s > rhs.size()) return false;
  // TODO: How to handle nulls!?
  for (size_type i=0; i<s; ++i)
    if (!((*this)[i].as<string>() < rhs[i].as<string>())) return false;
  return true;
}


void pqxx::result::swap(pqxx::result &other) throw ()
{
  const result *const l=m_l, *const r=m_r;
  internal::pq::PGresult *p(m_Result);

  m_l = other.m_l;
  m_r = other.m_r;
  m_Result = other.m_Result;
  other.m_l = l;
  other.m_r = r;
  other.m_Result = p;
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
  if (!m_Result)
    throw runtime_error("No result");

  string Err;

  switch (PQresultStatus(m_Result))
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
    Err = PQresultErrorMessage(m_Result);
    break;

  default:
    throw logic_error("libpqxx internal error: "
		      "pqxx::result: Unrecognized response code " +
		      to_string(int(PQresultStatus(m_Result))));
  }
  return Err;
}


void pqxx::result::MakeRef(internal::pq::PGresult *Other) throw ()
{
  m_Result = Other;
}


void pqxx::result::MakeRef(const pqxx::result &Other) throw ()
{
  m_l = &Other;
  m_r = Other.m_r;

  m_l->m_r = m_r->m_l = this;
  m_Result = Other.m_Result;
}


void pqxx::result::LoseRef() throw ()
{
  if ((m_l == this) && m_Result) PQclear(m_Result);
  m_Result = 0;
  m_l->m_r = m_r;
  m_r->m_l = m_l;
  m_l = m_r = this;
}



pqxx::result::size_type pqxx::result::affected_rows() const
{
  const char *const RowsStr = PQcmdTuples(m_Result);
  return RowsStr[0] ? atoi(RowsStr) : 0;
}


const char *pqxx::result::GetValue(pqxx::result::size_type Row, 
		                 pqxx::result::tuple::size_type Col) const
{
  return PQgetvalue(m_Result, Row, Col);
}


bool pqxx::result::GetIsNull(pqxx::result::size_type Row,
		           pqxx::result::tuple::size_type Col) const
{
  return PQgetisnull(m_Result, Row, Col) != 0;
}

pqxx::result::field::size_type 
pqxx::result::GetLength(pqxx::result::size_type Row,
                        pqxx::result::tuple::size_type Col) const
{
  return PQgetlength(m_Result, Row, Col);
}


int pqxx::result::errorposition() const throw ()
{
  int pos = -1;
#if defined(PQXX_HAVE_PQRESULTERRORFIELD)
  if (m_Result)
  {
    const char *p = PQresultErrorField(m_Result, PG_DIAG_STATEMENT_POSITION);
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
  const char *const N = PQfname(m_Result, Number);
  if (!N) 
    throw out_of_range("Invalid column number: " + to_string(Number));

  return N;
}


pqxx::result::tuple::size_type
pqxx::result::column_number(const char ColName[]) const
{
  const int N = PQfnumber(m_Result, ColName);
  if (N == -1)
    throw invalid_argument("Unknown column name: '" + string(ColName) + "'");

  return tuple::size_type(N);
}


// const_iterator

pqxx::result::const_iterator pqxx::result::const_iterator::operator++(int)
{
  const_iterator old(*this);
  m_Index++;
  return old;
}


pqxx::result::const_iterator pqxx::result::const_iterator::operator--(int)
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

