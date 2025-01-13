/* Basic type aliases and forward declarations.
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_TYPES
#define PQXX_H_TYPES

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <type_traits>


namespace pqxx
{
/// Number of rows in a result set.
using result_size_type = int;

/// Difference between result sizes.
using result_difference_type = int;

/// Number of fields in a row of database data.
using row_size_type = int;

/// Difference between row sizes.
using row_difference_type = int;

/// Number of bytes in a field of database data.
using field_size_type = std::size_t;

/// Number of bytes in a large object.
using large_object_size_type = int64_t;


// Forward declarations, to help break compilation dependencies.
// These won't necessarily include all classes in libpqxx.
class connection;
class const_result_iterator;
class const_reverse_result_iterator;
class const_reverse_row_iterator;
class const_row_iterator;
class dbtransaction;
// 9.0: Remove this.
class errorhandler;
class field;
class largeobjectaccess;
class notification_receiver;
struct range_error;
class result;
class row;
class stream_from;
class transaction_base;

/// Format code: is data text or binary?
/** Binary-compatible with libpq's format codes.
 */
enum class format : int
{
  text = 0,
  binary = 1,
};


/// Remove any constness, volatile, and reference-ness from a type.
/** @deprecated Use `std::remove_cvref` instead.
 */
template<typename TYPE> using strip_t = std::remove_cvref_t<TYPE>;


/// The type of a container's elements.
/** At the time of writing there's a similar thing in `std::experimental`,
 * which we may or may not end up using for this.
 */
template<std::ranges::range CONTAINER>
using value_type =
  std::remove_cvref_t<decltype(*std::begin(std::declval<CONTAINER>()))>;


/// Concept: Any type that we can read as a string of `char`.
template<typename STRING>
concept char_string =
  std::ranges::contiguous_range<STRING> and
  std::same_as<std::remove_cvref_t<value_type<STRING>>, char>;

/// Concept: Anything we can iterate to get things we can read as strings.
template<typename RANGE>
concept char_strings = std::ranges::range<RANGE> and
                       char_string<std::remove_cvref_t<value_type<RANGE>>>;

/// Concept: Anything we might want to treat as binary data.
template<typename DATA>
concept potential_binary =
  std::ranges::contiguous_range<DATA> and (sizeof(value_type<DATA>) == 1);


/// Concept: Binary string, akin to @c std::string for binary data.
/** Any type that satisfies this concept can represent an SQL BYTEA value.
 *
 * A @c binary has a @c begin(), @c end(), @c size(), and @data().  Each byte
 * is a @c std::byte, and they must all be laid out contiguously in memory so
 * we can reference them by a pointer.
 */
template<typename T>
concept binary =
  std::ranges::contiguous_range<T> and
  std::same_as<
    std::remove_cvref_t<std::ranges::range_reference_t<T>>, std::byte>;


/// A series of something that's not bytes.
template<typename T>
concept nonbinary_range =
  std::ranges::range<T> and
  not std::same_as<
    std::remove_cvref_t<std::ranges::range_reference_t<T>>, std::byte> and
  not std::same_as<
    std::remove_cvref_t<std::ranges::range_reference_t<T>>, char>;


/// Marker for @ref stream_from constructors: "stream from table."
/** @deprecated Use @ref stream_from::table() instead.
 */
struct from_table_t
{};

/// Marker for @ref stream_from constructors: "stream from query."
/** @deprecated Use @ref stream_from::query() instead.
 */
struct from_query_t
{};
} // namespace pqxx


namespace pqxx::internal
{
/// Concept: one of the "char" types.
template<typename T>
concept char_type = std::same_as<std::remove_cv_t<T>, char> or
                    std::same_as<std::remove_cv_t<T>, signed char> or
                    std::same_as<std::remove_cv_t<T>, unsigned char>;


/// Concept: an integral number type.
/** Unlike `std::integral`, this does not include the `char` types.
 */
template<typename T>
concept integer = std::integral<T> and not char_type<T>;
} // namespace pqxx::internal
#endif
