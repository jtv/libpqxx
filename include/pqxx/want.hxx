/* Helpers for constraining query results.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/want instead.
 *
 * Copyright (c) 2000-2024, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a flie called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_WANT
#define PQXX_H_WANT

#include <pqxx/except.hxx>
#include <pqxx/internal/concat.hxx>
#include <pqxx/result.hxx>


namespace pqxx
{
// XXX: Support "at least n rows"?
// XXX: Support dynamic limits.
// XXX: std::source_location?

/// Check that @ref result `r` contains exactly `expected` rows.
/** @throw unexpected_rows if the result did not contain exactly the expected
 * number of rows.
 */
template<result::size_type expected>
inline void want(result const &r)
{
  static_assert(expected >= 0);
  auto const sz{std::size(r)};
  PQXX_ASSUME(sz >= 0);
  if (sz != expected)
  {
    if constexpr (expected == 1)
      throw unexpected_rows{pqxx::internal::concat("Expected 1 row, got ", sz, ".")};
    else
      throw unexpected_rows{pqxx::internal::concat("Expected ", expected, " rows, got ", sz, ".")};
  }
}


/// Check that @ref result `r` contains an acceptable number of rows.
/** @throw unexpected_rows if `r` contains fewer than `minimum` rows,
 * or if it contained  
 */
template<result::size_type minimum, result::size_type excess>
inline void want(result const &r)
{
  static_assert(minimum >= 0);
  static_assert(excess > minimum);
  auto const sz{std::size(r)};
  PQXX_ASSUME(sz >= 0);
  if (sz < minimum)
  {
    if constexpr (minimum == 1)
      throw unexpected_rows{pqxx::internal::concat("Expected at least 1 row, got ", sz, ".")};
    else
      throw unexpected_rows{pqxx::internal::concat("Expected at least ", minimum, " rows, got ", sz, ".")};
  }
  if (sz >= excess)
  {
    if constexpr (excess == 1)
      throw unexpected_rows{pqxx::internal::concat("Expected no rows, got ", sz, ".")};
    else if constexpr (excess == 2)
      throw unexpected_rows{pqxx::internal::concat("Expected at most one row, got ", sz, ".")};
    else
      throw unexpected_rows{pqxx::internal::concat("Expected fewer than ", excess, " rows, got ", sz, ".")};
  }
}


/// Check that @ref result `r` contains exactly `expected` rows.
/** @throw unexpected_rows if the result did not contain exactly the expected
 * number of rows.
 */
inline void want(result::size_type expected, result const &r)
{
  auto const sz{std::size(r)};
  PQXX_ASSUME(sz >= 0);
  if (sz != expected)
  {
    if (expected == 1)
      throw unexpected_rows{pqxx::internal::concat("Expected 1 row, got ", sz, ".")};
    else
      throw unexpected_rows{pqxx::internal::concat("Expected ", expected, " rows, got ", sz, ".")};
  }
}


/// Check that @ref result `r` contains an acceptable number of rows.
/** @throw unexpected_rows if `r` contains fewer than `minimum` rows,
 * or if it contained  
 */
inline void want(result::size_type minimum, result::size_type excess, result const &r)
{
  auto const sz{std::size(r)};
  PQXX_ASSUME(sz >= 0);
  if (sz < minimum)
  {
    if (minimum == 1)
      throw unexpected_rows{pqxx::internal::concat("Expected at least 1 row, got ", sz, ".")};
    else
      throw unexpected_rows{pqxx::internal::concat("Expected at least ", minimum, " rows, got ", sz, ".")};
  }
  if (sz >= excess)
  {
    if (excess == 1)
      throw unexpected_rows{pqxx::internal::concat("Expected no rows, got ", sz, ".")};
    else if (excess == 2)
      throw unexpected_rows{pqxx::internal::concat("Expected at most one row, got ", sz, ".")};
    else
      throw unexpected_rows{pqxx::internal::concat("Expected fewer than ", excess, " rows, got ", sz, ".")};
  }
}
} // namespace pqxx

#endif
