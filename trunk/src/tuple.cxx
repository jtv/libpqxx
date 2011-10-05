/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tuple.cxx
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

#include "libpq-fe.h"

#include "pqxx/except"
#include "pqxx/result"


using namespace PGSTD;


pqxx::tuple::tuple(const result *r, size_t i) throw () :
  m_Home(r),
  m_Index(i),
  m_Begin(0),
  m_End(r ? r->columns() : 0)
{
}


pqxx::tuple::const_iterator pqxx::tuple::begin() const throw ()
{
  return const_iterator(*this, m_Begin);
}


pqxx::tuple::const_iterator pqxx::tuple::end() const throw ()
{
  return const_iterator(*this, m_End);
}


pqxx::tuple::reference pqxx::tuple::front() const throw ()
{
  return field(*this, m_Begin);
}


pqxx::tuple::reference pqxx::tuple::back() const throw ()
{
  return field(*this, m_End - 1);
}


pqxx::tuple::const_reverse_iterator pqxx::tuple::rbegin() const
{
  return const_reverse_tuple_iterator(end());
}


pqxx::tuple::const_reverse_iterator pqxx::tuple::rend() const
{
  return const_reverse_tuple_iterator(begin());
}


bool pqxx::tuple::operator==(const tuple &rhs) const throw ()
{
  if (&rhs == this) return true;
  const size_type s(size());
  if (rhs.size() != s) return false;
  // TODO: Depends on how null is handled!
  for (size_type i=0; i<s; ++i) if ((*this)[i] != rhs[i]) return false;
  return true;
}


pqxx::tuple::reference pqxx::tuple::operator[](size_type i) const throw ()
{
  return field(*this, m_Begin + i);
}


pqxx::tuple::reference pqxx::tuple::operator[](int i) const throw ()
{
  return operator[](size_type(i));
}


pqxx::tuple::reference pqxx::tuple::operator[](const char f[]) const
{
  return at(f);
}


pqxx::tuple::reference pqxx::tuple::operator[](const string &s) const
{
  return operator[](s.c_str());
}


pqxx::tuple::reference pqxx::tuple::at(int i) const throw (range_error)
{
  return at(size_type(i));
}


pqxx::tuple::reference pqxx::tuple::at(const string &s) const
{
  return at(s.c_str());
}


void pqxx::tuple::swap(tuple &rhs) throw ()
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


pqxx::field pqxx::tuple::at(const char f[]) const
{
  return field(*this, m_Begin + column_number(f));
}


pqxx::field pqxx::tuple::at(pqxx::tuple::size_type i) const
  throw (range_error)
{
  if (i >= size())
    throw range_error("Invalid field number");

  return operator[](i);
}


pqxx::oid pqxx::tuple::column_type(size_type ColNum) const
{
  return m_Home->column_type(m_Begin + ColNum);
}


pqxx::oid pqxx::tuple::column_table(size_type ColNum) const
{
  return m_Home->column_table(m_Begin + ColNum);
}


pqxx::tuple::size_type pqxx::tuple::table_column(size_type ColNum) const
{
  return m_Home->table_column(m_Begin + ColNum);
}


pqxx::tuple::size_type pqxx::tuple::column_number(const char ColName[]) const
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


pqxx::tuple::size_type pqxx::result::column_number(const char ColName[]) const
{
  const int N = PQfnumber(m_data, ColName);
  // TODO: Should this be an out_of_range?
  if (N == -1)
    throw argument_error("Unknown column name: '" + string(ColName) + "'");

  return tuple::size_type(N);
}


pqxx::tuple pqxx::tuple::slice(size_type Begin, size_type End) const
{
  if (Begin > End || End > size())
    throw range_error("Invalid field range");

  tuple result(*this);
  result.m_Begin = m_Begin + Begin;
  result.m_End = m_Begin + End;
  return result;
}


bool pqxx::tuple::empty() const throw ()
{
  return m_Begin == m_End;
}


pqxx::const_tuple_iterator pqxx::const_tuple_iterator::operator++(int)
{
  const_tuple_iterator old(*this);
  m_col++;
  return old;
}


pqxx::const_tuple_iterator pqxx::const_tuple_iterator::operator--(int)
{
  const_tuple_iterator old(*this);
  m_col--;
  return old;
}


pqxx::const_tuple_iterator
pqxx::const_reverse_tuple_iterator::base() const throw ()
{
  iterator_type tmp(*this);
  return ++tmp;
}


pqxx::const_reverse_tuple_iterator
pqxx::const_reverse_tuple_iterator::operator++(int)
{
  const_reverse_tuple_iterator tmp(*this);
  operator++();
  return tmp;
}


pqxx::const_reverse_tuple_iterator
pqxx::const_reverse_tuple_iterator::operator--(int)
{
  const_reverse_tuple_iterator tmp(*this);
  operator--();
  return tmp;
}
