/** Implementation of the pqxx::field class.
 *
 * pqxx::field refers to a field in a query result.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstring>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/field.hxx"
#include "pqxx/internal/libpq-forward.hxx"
#include "pqxx/result.hxx"
#include "pqxx/row.hxx"

#include "pqxx/internal/header-post.hxx"


pqxx::field::field(pqxx::row const &r, pqxx::row::size_type c) noexcept :
        m_home{r.m_result}, m_row{r.m_index}, m_col{c}
{}


PQXX_COLD bool pqxx::field::operator==(field const &rhs) const noexcept
{
  if (is_null() and rhs.is_null())
    return true;
  if (is_null() != rhs.is_null())
    return false;
  auto const s{size()};
  return (s == std::size(rhs)) and (std::memcmp(c_str(), rhs.c_str(), s) == 0);
}


char const *pqxx::field::name(sl loc) const &
{
  return home().column_name(column_number(), loc);
}


pqxx::row::size_type pqxx::field::table_column(sl loc) const
{
  return home().table_column(column_number(), loc);
}
