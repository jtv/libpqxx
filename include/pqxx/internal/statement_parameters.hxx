/** Common implementation for statement parameter lists.
 *
 * These are used for both prepared statements and parameterized statements.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY.  Other headers include it for you.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STATEMENT_PARAMETER
#define PQXX_H_STATEMENT_PARAMETER

#include <cstring>
#include <iterator>
#include <string>
#include <vector>

#include "pqxx/strconv.hxx"
#include "pqxx/util.hxx"


namespace pqxx::internal
{
template<typename ITERATOR>
constexpr inline auto const iterator_identity{
  [](decltype(*std::declval<ITERATOR>()) x) { return x; }};


/// Internal type: encode statement parameters.
/** Compiles arguments for prepared statements and parameterised queries into
 * a format that can be passed into libpq.
 *
 * Objects of this type are meant to be short-lived: a `c_params` lives and
 * dies entirely within the call to execute.  So, for example, if you pass in a
 * non-null pointer as a parameter, @ref params may simply use that pointer as
 * a parameter value, without arranging longer-term storage for the data to
 * which it points.  All values referenced by parameters must remain "live"
 * until the parameterised or prepared statement has been executed.
 */
struct PQXX_LIBEXPORT c_params final
{
  c_params() = default;
  /// Copying these objects is pointless and expensive.  Don't do it.
  c_params(c_params const &) = delete;
  c_params(c_params &&) = default;

  /// Pre-allocate storage for `n` parameters.
  void reserve(std::size_t n) &;

  /// As used by libpq: pointers to parameter values.
  std::vector<char const *> values;
  /// As used by libpq: lengths of non-null arguments, in bytes.
  std::vector<int> lengths;
  /// As used by libpq: effectively boolean "is this a binary parameter?"
  std::vector<format> formats;
};
} // namespace pqxx::internal
#endif
