/*-------------------------------------------------------------------------
 *
 *   FILE
 *	row.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::result class and support classes.
 *   pqxx::result represents the set of result rows from a database query
 *
 * Copyright (c) 2001-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


pqxx::row::row(const result *r, size_t i) PQXX_NOEXCEPT :
  m_Home(r),
  m_Index(i),
  m_Begin(0),
  m_End(r ? r->columns() : 0)
{
}


pqxx::row::const_iterator pqxx::row::begin() const PQXX_NOEXCEPT
{
  return const_iterator(*this, m_Begin);
}


pqxx::row::const_iterator pqxx::row::cbegin() const PQXX_NOEXCEPT
{
  return begin();
}


pqxx::row::const_iterator pqxx::row::end() const PQXX_NOEXCEPT
{
  return const_iterator(*this, m_End);
}


pqxx::row::const_iterator pqxx::row::cend() const PQXX_NOEXCEPT
{
  return end();
}


pqxx::row::reference pqxx::row::front() const PQXX_NOEXCEPT
{
  return field(*this, m_Begin);
}


pqxx::row::reference pqxx::row::back() const PQXX_NOEXCEPT
{
  return field(*this, m_End - 1);
}


pqxx::row::const_reverse_iterator pqxx::row::rbegin() const
{
  return const_reverse_row_iterator(end());
}


pqxx::row::const_reverse_iterator pqxx::row::crbegin() const
{
  return rbegin();
}


pqxx::row::const_reverse_iterator pqxx::row::rend() const
{
  return const_reverse_row_iterator(begin());
}


pqxx::row::const_reverse_iterator pqxx::row::crend() const
{
  return rend();
}


bool pqxx::row::operator==(const row &rhs) const PQXX_NOEXCEPT
{
  if (&rhs == this) return true;
  const size_type s(size());
  if (rhs.size() != s) return false;
  // TODO: Depends on how null is handled!
  for (size_type i=0; i<s; ++i) if ((*this)[i] != rhs[i]) return false;
  return true;
}


pqxx::row::reference pqxx::row::operator[](size_type i) const PQXX_NOEXCEPT
{
  return field(*this, m_Begin + i);
}


pqxx::row::reference pqxx::row::operator[](int i) const PQXX_NOEXCEPT
{
  return operator[](size_type(i));
}


pqxx::row::reference pqxx::row::operator[](const char f[]) const
{
  return at(f);
}


pqxx::row::reference pqxx::row::operator[](const std::string &s) const
{
  return operator[](s.c_str());
}


pqxx::row::reference pqxx::row::at(int i) const
{
  return at(size_type(i));
}


pqxx::row::reference pqxx::row::at(const std::string &s) const
{
  return at(s.c_str());
}


void pqxx::row::swap(row &rhs) PQXX_NOEXCEPT
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


pqxx::field pqxx::row::at(const char f[]) const
{
  return field(*this, m_Begin + column_number(f));
}


pqxx::field pqxx::row::at(pqxx::row::size_type i) const
{
  if (i >= size())
    throw range_error("Invalid field number");

  return operator[](i);
}


pqxx::oid pqxx::row::column_type(size_type ColNum) const
{
  return m_Home->column_type(m_Begin + ColNum);
}


pqxx::oid pqxx::row::column_table(size_type ColNum) const
{
  return m_Home->column_table(m_Begin + ColNum);
}


pqxx::row::size_type pqxx::row::table_column(size_type ColNum) const
{
  return m_Home->table_column(m_Begin + ColNum);
}


pqxx::row::size_type pqxx::row::column_number(const char ColName[]) const
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


pqxx::row::size_type pqxx::result::column_number(const char ColName[]) const
{
  const int N = PQfnumber(m_data, ColName);
  // TODO: Should this be an out_of_range?
  if (N == -1)
    throw argument_error("Unknown column name: '" + std::string(ColName) + "'");

  return row::size_type(N);
}


pqxx::row pqxx::row::slice(size_type Begin, size_type End) const
{
  if (Begin > End || End > size())
    throw range_error("Invalid field range");

  row result(*this);
  result.m_Begin = m_Begin + Begin;
  result.m_End = m_Begin + End;
  return result;
}


bool pqxx::row::empty() const PQXX_NOEXCEPT
{
  return m_Begin == m_End;
}


pqxx::const_row_iterator pqxx::const_row_iterator::operator++(int)
{
  const_row_iterator old(*this);
  m_col++;
  return old;
}


pqxx::const_row_iterator pqxx::const_row_iterator::operator--(int)
{
  const_row_iterator old(*this);
  m_col--;
  return old;
}


pqxx::const_row_iterator
pqxx::const_reverse_row_iterator::base() const PQXX_NOEXCEPT
{
  iterator_type tmp(*this);
  return ++tmp;
}


pqxx::const_reverse_row_iterator
pqxx::const_reverse_row_iterator::operator++(int)
{
  const_reverse_row_iterator tmp(*this);
  operator++();
  return tmp;
}


pqxx::const_reverse_row_iterator
pqxx::const_reverse_row_iterator::operator--(int)
{
  const_reverse_row_iterator tmp(*this);
  operator--();
  return tmp;
}
