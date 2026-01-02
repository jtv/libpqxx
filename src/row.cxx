/** Implementation of the pqxx::result class and support classes.
 *
 * pqxx::result represents the set of result rows from a database query.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstdlib>
#include <cstring>

#include "pqxx/internal/header-pre.hxx"

extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/except.hxx"
#include "pqxx/result.hxx"
#include "pqxx/row.hxx"

#include "pqxx/internal/header-post.hxx"


pqxx::row::row(result r, result::size_type index, size_type cols) noexcept :
        m_result{std::move(r)}, m_index{index}, m_end{cols}
{}


pqxx::field_ref pqxx::row::front() const noexcept
{
  return {m_result, m_index, 0};
}


pqxx::field_ref pqxx::row::back() const noexcept
{
  return {m_result, m_index, m_end - 1};
}


pqxx::field_ref pqxx::row::operator[](size_type i) const noexcept
{
  return {m_result, m_index, i};
}


#if defined(PQXX_HAVE_MULTIDIM)
pqxx::field_ref pqxx::row_ref::operator[](zview col_name, sl loc) const
{
  return at(col_name, loc);
}
#endif // PQXX_HAVE_MULTIDIM


pqxx::field_ref pqxx::row_ref::operator[](zview col_name) const
{
  return at(col_name, sl::current());
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


pqxx::field_ref pqxx::row::at(zview col_name, sl loc) const
{
  return {m_result, m_index, column_number(col_name, loc)};
}


pqxx::field_ref pqxx::row::at(pqxx::row::size_type i, sl loc) const
{
  auto const sz{this->size()};
  if (std::cmp_greater_equal(i, sz))
    throw range_error{
      std::format(
        "Column index out of range: {} in a result of {} column(s).", i, sz),
      loc};
  return operator[](i);
}


pqxx::const_row_iterator pqxx::const_row_iterator::operator++(int) & noexcept
{
  auto old{*this};
  pqxx::internal::gate::field_ref_const_row_iterator(m_field).offset(1);
  return old;
}


pqxx::const_row_iterator pqxx::const_row_iterator::operator--(int) & noexcept
{
  auto old{*this};
  pqxx::internal::gate::field_ref_const_row_iterator(m_field).offset(-1);
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
