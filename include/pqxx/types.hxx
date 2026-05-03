/* Basic type aliases and forward declarations.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_TYPES_HXX
#define PQXX_TYPES_HXX

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <cstddef>
#include <cstdint>
#include <iterator>

#if defined(PQXX_HAVE_CONCEPTS) && defined(PQXX_HAVE_RANGES)
#  include <ranges>
#endif


namespace pqxx::internal
{
/// Attempt to demangle @c std::type_info::name() to something human-readable.
PQXX_LIBEXPORT std::string demangle_type_name(char const[]);
} // namespace pqxx::internal


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
class binarystring;
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
/** @deprecated In C++20 we'll replace this with std::remove_cvref.
 */
template<typename TYPE>
using strip_t = std::remove_cv_t<std::remove_reference_t<TYPE>>;


#if defined(PQXX_HAVE_CONCEPTS) && defined(PQXX_HAVE_RANGES)
/// The type of a container's elements.
/** At the time of writing there's a similar thing in `std::experimental`,
 * which we may or may not end up using for this.
 */
template<std::ranges::range CONTAINER>
using value_type = strip_t<decltype(*std::begin(std::declval<CONTAINER>()))>;
#else  // PQXX_HAVE_CONCEPTS
/// The type of a container's elements.
/** At the time of writing there's a similar thing in `std::experimental`,
 * which we may or may not end up using for this.
 */
template<typename CONTAINER>
using value_type = strip_t<decltype(*std::begin(std::declval<CONTAINER>()))>;
#endif // PQXX_HAVE_CONCEPTS


#if defined(PQXX_HAVE_CONCEPTS) && defined(PQXX_HAVE_RANGES)
/// Concept: Any type that we can read as a string of `char`.
template<typename STRING>
concept char_string = std::ranges::contiguous_range<STRING> and
                      std::same_as<strip_t<value_type<STRING>>, char>;

/// Concept: Anything we can iterate to get things we can read as strings.
template<typename RANGE>
concept char_strings =
  std::ranges::range<RANGE> and char_string<strip_t<value_type<RANGE>>>;

/// Concept: Anything we might want to treat as binary data.
template<typename DATA>
concept potential_binary =
  std::ranges::contiguous_range<DATA> and (sizeof(value_type<DATA>) == 1);
#endif // PQXX_HAVE_CONCEPTS


// C++20: Retire these compatibility definitions.
#if defined(PQXX_HAVE_CONCEPTS) && defined(PQXX_HAVE_RANGES)

/// Template argument type for a range.
/** This is a concept, so only available in C++20 or better.  In pre-C++20
 * environments it's just an alias for @ref typename.
 */
#  define PQXX_RANGE_ARG std::ranges::range

/// Template argument type for @ref char_string.
/** This is a concept, so only available in C++20 or better.  In pre-C++20
 * environments it's just an alias for @ref typename.
 */
#  define PQXX_CHAR_STRING_ARG pqxx::char_string

/// Template argument type for @ref char_strings
/** This is a concept, so only available in C++20 or better.  In pre-C++20
 * environments it's just an alias for @ref typename.
 */
#  define PQXX_CHAR_STRINGS_ARG pqxx::char_strings

#else // PQXX_HAVE_CONCEPTS

/// Template argument type for a range.
/** This is a concept, so only available in C++20 or better.  In pre-C++20
 * environments it's just an alias for @ref typename.
 */
#  define PQXX_RANGE_ARG typename

/// Template argument type for @ref char_string.
/** This is a concept, so only available in C++20 or better.  In pre-C++20
 * environments it's just an alias for @ref typename.
 */
#  define PQXX_CHAR_STRING_ARG typename

/// Template argument type for @ref char_strings
/** This is a concept, so only available in C++20 or better.  In pre-C++20
 * environments it's just an alias for @ref typename.
 */
#  define PQXX_CHAR_STRINGS_ARG typename

#endif // PQXX_HAVE_CONCEPTS

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
std::string const type_name{internal::demangle_type_name(typeid(TYPE).name())};


/// Return human-readable name for `TYPE`.
template<typename TYPE> inline constexpr std::string_view name_type() noexcept
{
  return type_name<TYPE>;
}


// Specialisations of name_type<>() follow.  This may avoid problems with gcc
// 15, where definitions of `type_name<...>` (the templated global variable)
// in the library and in the headers are not considered identical, leading to
// a double free.
//
// We work around this by specialising the function for the built-in types, so
// that we may never need type_name.

template<>
PQXX_PURE constexpr inline std::string_view name_type<std::string>() noexcept
{
  return "std::string";
}
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
template<>
PQXX_PURE constexpr inline std::string_view name_type<bool>() noexcept
{
  return "bool";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<short>() noexcept
{
  return "short";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<int>() noexcept
{
  return "int";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<long>() noexcept
{
  return "long";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<long long>() noexcept
{
  return "long long";
}
template<>
PQXX_PURE constexpr inline std::string_view
name_type<unsigned short>() noexcept
{
  return "unsigned short";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<unsigned>() noexcept
{
  return "unsigned";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<unsigned long>() noexcept
{
  return "unsigned long";
}
template<>
PQXX_PURE constexpr inline std::string_view
name_type<unsigned long long>() noexcept
{
  return "unsigned long long";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<float>() noexcept
{
  return "float";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<double>() noexcept
{
  return "double";
}
template<>
PQXX_PURE constexpr inline std::string_view name_type<long double>() noexcept
{
  return "long double";
}
template<>
PQXX_PURE constexpr inline std::string_view
name_type<std::nullptr_t>() noexcept
{
  return "std::nullptr_t";
}

} // namespace pqxx
#endif
