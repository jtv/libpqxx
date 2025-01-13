/** Implementation of the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query.
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstdlib>
#include <cstring>

extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/except.hxx"
#include "pqxx/result.hxx"
#include "pqxx/row.hxx"

#include "pqxx/internal/header-post.hxx"


pqxx::row::row(result r, result::size_type index, size_type cols) noexcept :
        m_result{std::move(r)}, m_index{index}, m_end{cols}
{}


pqxx::row::const_iterator pqxx::row::begin() const noexcept
{
  return {*this, 0};
}


pqxx::row::const_iterator pqxx::row::cbegin() const noexcept
{
  return begin();
}


pqxx::row::const_iterator pqxx::row::end() const noexcept
{
  return {*this, m_end};
}


pqxx::row::const_iterator pqxx::row::cend() const noexcept
{
  return end();
}


pqxx::row::reference pqxx::row::front() const noexcept
{
  return field{m_result, m_index, 0};
}


pqxx::row::reference pqxx::row::back() const noexcept
{
  return field{m_result, m_index, m_end - 1};
}


pqxx::row::const_reverse_iterator pqxx::row::rbegin() const noexcept
{
  return const_reverse_row_iterator{end()};
}


pqxx::row::const_reverse_iterator pqxx::row::crbegin() const noexcept
{
  return rbegin();
}


pqxx::row::const_reverse_iterator pqxx::row::rend() const noexcept
{
  return const_reverse_row_iterator{begin()};
}


pqxx::row::const_reverse_iterator pqxx::row::crend() const noexcept
{
  return rend();
}


bool pqxx::row::operator==(row const &rhs) const noexcept
{
  if (&rhs == this)
    return true;
  auto const s{size()};
  if (std::size(rhs) != s)
    return false;
  for (size_type i{0}; i < s; ++i)
    if ((*this)[i] != rhs[i])
      return false;
  return true;
}


pqxx::row::reference pqxx::row::operator[](size_type i) const noexcept
{
  return field{m_result, m_index, i};
}


pqxx::row::reference pqxx::row::operator[](zview col_name) const
{
  return at(col_name);
}


void pqxx::row::swap(row &rhs) noexcept
{
  auto const i{m_index};
  auto const e{m_end};
  m_result.swap(rhs.m_result);
  m_index = rhs.m_index;
  m_end = rhs.m_end;
  rhs.m_index = i;
  rhs.m_end = e;
}


pqxx::field pqxx::row::at(zview col_name) const
{
  return {m_result, m_index, column_number(col_name)};
}


pqxx::field pqxx::row::at(pqxx::row::size_type i) const
{
  if (i >= size())
    throw range_error{"Invalid field number."};

  return operator[](i);
}


pqxx::oid pqxx::row::column_type(size_type col_num) const
{
  return m_result.column_type(col_num);
}


pqxx::oid pqxx::row::column_table(size_type col_num) const
{
  return m_result.column_table(col_num);
}


pqxx::row::size_type pqxx::row::table_column(size_type col_num) const
{
  return m_result.table_column(col_num);
}


pqxx::row::size_type pqxx::row::column_number(zview col_name) const
{
  return m_result.column_number(col_name);
}


pqxx::const_row_iterator pqxx::const_row_iterator::operator++(int) & noexcept
{
  auto old{*this};
  m_col++;
  return old;
}


pqxx::const_row_iterator pqxx::const_row_iterator::operator--(int) & noexcept
{
  auto old{*this};
  m_col--;
  return old;
}


pqxx::const_row_iterator
pqxx::const_reverse_row_iterator::base() const noexcept
{
  iterator_type tmp{*this};
  return ++tmp;
}


pqxx::const_reverse_row_iterator
pqxx::const_reverse_row_iterator::operator++(int) & noexcept
{
  auto tmp{*this};
  operator++();
  return tmp;
}


pqxx::const_reverse_row_iterator
pqxx::const_reverse_row_iterator::operator--(int) &
{
  auto tmp{*this};
  operator--();
  return tmp;
}
