/* Basic type aliases and forward declarations.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen
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
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>

#if defined(PQXX_HAVE_STACKTRACE)
#  include <stacktrace>
#endif

#if defined(PQXX_HAVE_TYPE_DISPLAY)
#  include <meta>
#endif


namespace pqxx
{
/// Convenience alias for `std::source_location`.  It's just too long.
using sl = std::source_location;

#if defined(PQXX_HAVE_STACKTRACE)
/// Alias for `std::stacktrace`, for brevity.
using st = std::stacktrace;
#else
/// There is no `std::stacktrace` on this system.  Use a placeholder.
struct stacktrace_placeholder final
{
  /// Placeholder for `std::stacktrace::current()`.
  [[nodiscard]] PQXX_PURE static constexpr stacktrace_placeholder
  current() noexcept
  {
    return {};
  }
};
/// Placeholder for future `std::stacktrace`.
using st = stacktrace_placeholder;
#endif

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
 *
 * Why use an `int` for this when a single bit would be enough?  Because this
 * maps directly to the C-level values used in libpq.
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
using value_type = std::remove_cvref_t<std::ranges::range_value_t<CONTAINER>>;


/// A type one byte in size.
template<typename CHAR>
concept char_sized = (sizeof(CHAR) == 1);


/// Concept: Any type that we can read as a string of `char`.
template<typename STRING>
concept char_string = std::ranges::contiguous_range<STRING> and
                      std::same_as<std::remove_cv_t<value_type<STRING>>, char>;


/// Concept: Anything we can iterate to get things we can read as strings.
template<typename RANGE>
concept char_strings = std::ranges::range<RANGE> and
                       char_string<std::remove_cv_t<value_type<RANGE>>>;


/// Concept: Anything we might want to treat as binary data.
template<typename DATA>
concept potential_binary =
  std::ranges::contiguous_range<DATA> and char_sized<value_type<DATA>> and
  not std::is_reference_v<value_type<DATA>>;


/// Concept: Binary string, akin to @c std::string for binary data.
/** Any type that satisfies this concept can represent an SQL BYTEA value.
 *
 * A @c binary has a @c begin(), @c end(), @c size(), and @data().  Each byte
 * is a @c std::byte, and they must all be laid out contiguously in memory so
 * we can reference them by a pointer.
 */
template<typename T>
concept binary = std::ranges::contiguous_range<T> and
                 std::same_as<std::remove_cv_t<value_type<T>>, std::byte>;


/// A series of something that's not bytes.
template<typename T>
concept nonbinary_range =
  std::ranges::range<T> and
  not std::same_as<
    std::remove_cvref_t<std::ranges::range_reference_t<T>>, std::byte> and
  not std::same_as<
    std::remove_cvref_t<std::ranges::range_reference_t<T>>, char>;


/// Type alias for a view of bytes.
using bytes_view = std::span<std::byte const>;


/// Type alias for a view of writable bytes.
using writable_bytes_view = std::span<std::byte>;


/// Concept: A value that's not just a reference to values elsewhere.
/** This can be an important distinction when returning values.  For example,
 * if a function creates a `std::string` in a local variable, it can't then
 * return a `std::string_view` referring to that string.  By the time the
 * caller gets to it, the underlying data is no longer valid.
 *
 * In most cases these are decisions we make while writing code.  But when
 * converting data to a caller-selected type, there are situations where it's
 * safe to return a view and there are situations where it's not.
 */
template<typename T>
concept not_borrowed =
  not std::is_reference_v<T> and not std::is_pointer_v<T> and
  not std::ranges::borrowed_range<T>;


/// Concept: A C++ `enum` type.
template<typename E>
concept enum_type = std::is_enum_v<E>;


/// Marker for @ref stream_from constructors: "stream from table."
/** @deprecated Use @ref stream_from::table() instead.
 */
struct from_table_t final
{};

/// Marker for @ref stream_from constructors: "stream from query."
/** @deprecated Use @ref stream_from::query() instead.
 */
struct from_query_t final
{};
} // namespace pqxx


namespace pqxx::internal
{
#if !defined(PQXX_HAVE_TYPE_DISPLAY)
/// Attempt to demangle @c std::type_info::name() to something human-readable.
PQXX_LIBEXPORT PQXX_ZARGS std::string demangle_type_name(char const[]);
#endif
} // namespace pqxx::internal


namespace pqxx
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
#if defined(PQXX_HAVE_TYPE_DISPLAY)
/// A human-readable name for a type, used in error messages and such.
template<typename TYPE>
[[deprecated("Use name_type() instead.")]]
std::string const type_name{display_string_of(^^TYPE)};

#else

/// A human-readable name for a type, used in error messages and such.
/** Actually this may not always be very user-friendly.  It uses
 * @c std::type_info::name().  On gcc-like compilers we try to demangle its
 * output.  Visual Studio produces human-friendly names out of the box.
 *
 * This variable is not inline.  Inlining it gives rise to "memory leak"
 * warnings from asan, the address sanitizer, possibly from use of
 * @c std::type_info::name.
 */
template<typename TYPE>
[[deprecated("Use name_type() instead.")]]
std::string const type_name{
  pqxx::internal::demangle_type_name(typeid(TYPE).name())};

#endif
#include "pqxx/internal/ignore-deprecated-post.hxx"

/// Return human-readable name for `TYPE`.
template<typename TYPE> inline constexpr std::string_view name_type() noexcept
{
#if defined(PQXX_HAVE_TYPE_DISPLAY)

  return display_string_of(^^TYPE);

#else

#  include "pqxx/internal/ignore-deprecated-pre.hxx"
  return type_name<TYPE>;
#  include "pqxx/internal/ignore-deprecated-post.hxx"

#endif
}


// Specialisations of name_type<>() follow.  Some are tested, but because they
// are so trivial, the coverage tools never notice that they get tested.
// There's just no point running coverage for these.
// LCOV_EXCL_START

/// Specialisation to save on startup work & produce friendlier output.
template<>
PQXX_PURE constexpr inline std::string_view name_type<std::string>() noexcept
{
  return "std::string";
}
/// Specialisation to save on startup work & produce friendlier output.
template<>
PQXX_PURE constexpr inline std::string_view
name_type<std::string_view>() noexcept
{
  return "std::string_view";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<char const *>() noexcept
{
  return "char const *";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<bool>() noexcept
{
  return "bool";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<short>() noexcept
{
  return "short";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<int>() noexcept
{
  return "int";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<long>() noexcept
{
  return "long";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<long long>() noexcept
{
  return "long long";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view
name_type<unsigned short>() noexcept
{
  return "unsigned short";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<unsigned>() noexcept
{
  return "unsigned";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<unsigned long>() noexcept
{
  return "unsigned long";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view
name_type<unsigned long long>() noexcept
{
  return "unsigned long long";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<float>() noexcept
{
  return "float";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<double>() noexcept
{
  return "double";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view name_type<long double>() noexcept
{
  return "long double";
}
/// Specialisation to save on startup work.
template<>
PQXX_PURE constexpr inline std::string_view
name_type<std::nullptr_t>() noexcept
{
  return "std::nullptr_t";
}

// LCOV_EXCL_STOP
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
