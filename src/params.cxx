/* Implementations related to prepared and parameterised statements.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/connection.hxx"
#include "pqxx/params.hxx"
#include "pqxx/transaction_base.hxx"

#include "pqxx/internal/header-post.hxx"


pqxx::encoding_group
pqxx::internal::get_encoding_group(pqxx::connection const &cx, sl loc)
{
  return cx.get_encoding_group(loc);
}


pqxx::encoding_group
pqxx::internal::get_encoding_group(pqxx::transaction_base const &tx, sl loc)
{
  return get_encoding_group(tx.conn(), loc);
}


void pqxx::internal::c_params::reserve(std::size_t n) &
{
  values.reserve(n);
  lengths.reserve(n);
  formats.reserve(n);
}


void pqxx::params::reserve(std::size_t n) &
{
  m_params.reserve(n);
}


void pqxx::params::append(sl) &
{
  m_params.emplace_back(nullptr);
}


void pqxx::params::append(zview value, sl) &
{
  m_params.emplace_back(value);
}


void pqxx::params::append(std::string const &value, sl) &
{
  m_params.emplace_back(value);
}


void pqxx::params::append(std::string &&value, sl) &
{
  m_params.emplace_back(std::move(value));
}


void pqxx::params::append(params const &value, sl) &
{
  this->reserve(std::size(value.m_params) + std::size(this->m_params));
  for (auto const &param : value.m_params) m_params.emplace_back(param);
}


void pqxx::params::append(bytes_view value, sl) &
{
  m_params.emplace_back(value);
}


void pqxx::params::append(bytes &&value, sl) &
{
  m_params.emplace_back(std::move(value));
}


void pqxx::params::append(params &&value, sl) &
{
  // TODO: If currently empty, just "steal" value's m_params wholesale.
  // C++23: Use append_range()?
  this->reserve(std::size(value.m_params) + std::size(this->m_params));
  for (auto const &param : value.m_params) m_params.emplace_back(param);
  value.m_params.clear();
}


pqxx::internal::c_params pqxx::params::make_c_params(sl loc) const
{
  pqxx::internal::c_params p;
  p.reserve(std::size(m_params));
  for (auto const &param : m_params)
    std::visit(
      [&p, &loc](auto const &value) {
        using T = std::remove_cvref_t<decltype(value)>;

        if constexpr (std::is_same_v<T, std::nullptr_t>)
        {
          p.values.push_back(nullptr);
          p.lengths.push_back(0);
        }
        else
        {
          p.values.push_back(reinterpret_cast<char const *>(std::data(value)));
          p.lengths.push_back(
            check_cast<int>(std::ssize(value), s_overflow, loc));
        }

        p.formats.push_back(param_format(value));
      },
      param);

  return p;
}
