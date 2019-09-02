/* String conversion definitions.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stringconv instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_STRINGCONV
#define PQXX_H_STRINGCONV

#include "pqxx/compiler-public.hxx"

#include <algorithm>
#include <cstring>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <typeinfo>

#if __has_include(<charconv>)
#include <charconv>
#endif

#include "pqxx/except.hxx"


namespace pqxx::internal
{
/// Implementation classes for @c str.
/** We can't define these directly as @c str because of the way
 * @c std::enable_if works: to make that work, we need an extra template
 * parameter, which then seems to break template deduction when we define a
 * @c str{value}.
 */
template<typename T, typename E> class str_impl;


/// Attempt to demangle @c std::type_info::name() to something human-readable.
PQXX_LIBEXPORT std::string demangle_type_name(const char[]);
} // namespace pqxx::internal


namespace pqxx
{
/// Marker-type wrapper: zero-terminated @c std::string_view.
/** This is basically a @c std::string_view, but it adds the guarantee that
 * if its data pointer is non-null, there is a terminating zero byte right
 * after the contents.
 *
 * This means that it can also be used as a C-style string, which often matters
 * since libpqxx builds on top of a C library.  Therefore, it also adds a
 * @c c_str method.
 */
class PQXX_LIBEXPORT zview : public std::string_view
{
public:
  template<typename ...Args> constexpr zview(Args &&...args) :
    std::string_view(std::forward<Args>(args)...)
  {}

  /// Either a null pointer, or a zero-terminated text buffer.
  constexpr const char *c_str() noexcept { return data(); }
};


/**
 * @defgroup stringconversion String conversion
 *
 * The PostgreSQL server accepts and represents data in string form.  It has
 * its own formats for various data types.  The string conversions define how
 * various C++ types translate to and from their respective PostgreSQL text
 * representations.
 *
 * Each conversion is defined by specialisations of certain templates:
 * @c string_traits, @c to_buf, and @c str.  It gets complicated if you want
 * top performance, but until you do, all you really need to care about when
 * converting values between C++ in-memory representations such as @c int and
 * the postgres string representations is the @c pqxx::to_string and
 * @c pqxx::from_string functions.
 *
 * If you need to convert a type which is not supported out of the box, you'll
 * need to define your own specialisations for these templates, similar to the
 * ones defined here and in `pqxx/conversions.hxx`.  Any conversion code which
 * "sees" your specialisation will now support your conversion.  In particular,
 * you'll be able to read result fields into a variable of the new type.
 *
 * There is a macro to help you define conversions for individual enumeration
 * types.  The conversion will represent enumeration values as numeric strings.
 */
//@{

/// A human-readable name for a type, used in error messages and such.
/** Actually this may not always be @i very user-friendly.  It uses
 * @c std::type_info::name().  On gcc-like compilers we try to demangle its
 * output.  Visual Studio produces human-friendly names out of the box.
 *
 * This variable is not inline.  Inlining it gives rise to "memory leak"
 * warnings from asan, the address sanitizer, possibly from use of
 * @c std::type_info::name.
 */
template<typename TYPE> const std::string type_name{
	internal::demangle_type_name(typeid(TYPE).name())};


/// @addtogroup stringconversion
//@{
/// Traits class for use in string conversions.
/** Specialize this template for a type for which you wish to add to_string
 * and from_string support.  It indicates whether the type has a natural null
 * value (if not, consider using @c std::optional for that), and whether a
 * given value is null, and so on.
 */
template<typename T, typename = void> struct string_traits;


/// Return a @c string_view representing value, plus terminating zero.
/** Produces a @c string_view, whose @c data() will be null if @c value was
 * null.  Otherwise, it will contain the PostgreSQL string representation for
 * @c value.
 *
 * Uses the space from @c begin to @c end as a buffer, if needed.  The
 * returned string may lie somewhere in that buffer, or it may be a
 * compile-time constant, or it may be null if value was a null value.  Even
 * if the string is stored in the buffer, its @c begin() may or may not be
 * the same as @c begin.
 *
 * The @c string_view is guaranteed to be valid as long as the buffer from
 * @c begin to @c end remains accessible and unmodified.
 *
 * @throws pqxx::conversion_overrun if the provided buffer space may not be
 * enough.  For maximum performance, this is a conservative estimate.  It may
 * complain about a buffer which is actually large enough for your value, if
 * an exact check gets too expensive.
 */
template<typename T> inline zview
to_buf(char *begin, char *end, const T &value);


/// Value-to-string converter: represent value as a postgres-compatible string.
/** @warning This feature is experimental.  It may change, or disappear.
 *
 * Turns a value of (more or less) any type into its PostgreSQL string
 * representation.  It keeps the string representation "alive" in memory while
 * the @c str object exists.  After that, accessing the string becomes
 * undefined.
 *
 * @c warning The value cannot be null.
 *
 * In situations where convenience matters more than performance, use the
 * @c to_string functions which create and return a @c std::string.  It's
 * expensive but convenient.  If you need extreme memory efficiency, consider
 * using @c to_buf and allocating your own buffer.  In the space between those
 * two extremes, use @c str.
 */
template<typename T> class str : internal::str_impl<T, void>
{
public:
  str() =delete;
  str(const str &) =delete;
  str(str &&) =delete;

  explicit str(const T &value) : internal::str_impl<T, void>{value} {}
  explicit str(T &value) : internal::str_impl<T, void>{value} {}

  str &operator=(const str &) =delete;
  str &operator=(str &&) =delete;

  using internal::str_impl<T, void>::view;
  using internal::str_impl<T, void>::c_str;
  operator zview() const { return view(); }
};


/// Helper class for defining enum conversions.
/** The conversion will convert enum values to numeric strings, and vice versa.
 *
 * To define a string conversion for an enum type, derive a @c string_traits
 * specialisation for the enum from this struct.
 *
 * There's usually an easier way though: the @c PQXX_DECLARE_ENUM_CONVERSION
 * macro.  Use @c enum_traits manually only if you need to customise your
 * traits type in more detail, e.g. if your enum has a "null" value built in.
 */
template<typename ENUM>
struct enum_traits
{
  static constexpr bool has_null = false;

  static ENUM from_string(std::string_view text)
  {
    using impl_type = std::underlying_type_t<ENUM>;
    return static_cast<ENUM>(string_traits<impl_type>::from_string(text));
  }
};


/// Macro: Define a string conversion for an enum type.
/** This specialises the @c pqxx::string_traits template, so use it in the
 * @c ::pqxx namespace.
 *
 * For example:
 *
 *      #include <iostream>
 *      #include <pqxx/strconv>
 *      enum X { xa, xb };
 *      namespace pqxx { PQXX_DECLARE_ENUM_CONVERSION(x); }
 *      int main() { std::cout << pqxx::to_string(xa) << std::endl; }
 */
#define PQXX_DECLARE_ENUM_CONVERSION(ENUM) \
template<> [[maybe_unused]] zview inline \
to_buf(char *begin, char *end, const ENUM &value) \
{ return to_buf(begin, end, std::underlying_type_t<ENUM>(value)); } \
template<> struct string_traits<ENUM> : pqxx::enum_traits<ENUM> {}; \
template<> const std::string type_name<ENUM>{#ENUM}


/// Attempt to convert postgres-generated string to given built-in type.
/** If the form of the value found in the string does not match the expected
 * type, e.g. if a decimal point is found when converting to an integer type,
 * the conversion fails.  Overflows (e.g. converting "9999999999" to a 16-bit
 * C++ type) are also treated as errors.  If in some cases this behaviour should
 * be inappropriate, convert to something bigger such as @c long @c int first
 * and then truncate the resulting value.
 *
 * Only the simplest possible conversions are supported.  Fancy features like
 * hexadecimal or octal, spurious signs, or exponent notation won't work.
 * Whitespace is not stripped away.  Only the kinds of strings that come out of
 * PostgreSQL and out of to_string() can be converted.
 */
template<typename T> inline T from_string(std::string_view text)
{ return string_traits<T>::from_string(text); }


/// Attempt to convert postgres-generated string to given built-in object.
/** This is like the single-argument form of the function, except instead of
 * returning the value, it sets @c obj to the value.
 *
 * You may find this more convenient in that it infers the type you want from
 * the argument you pass.  But there are disadvantages: it requires an
 * assignment operator, and it may be less efficient.
 */
template<typename T> inline void from_string(std::string_view text, T &obj)
{ obj = from_string<T>(text); }


/// Convert a value to a readable string that PostgreSQL will understand.
/** This is the convenient way to represent a value as text.  It's also fairly
 * expensive, since it creates a @c std::string.  The @c pqxx::str class is a
 * more efficient but slightly less convenient alternative.  Probably.
 * Depending on the type of value you're trying to convert.
 *
 * The conversion does no special formatting, and ignores any locale settings.
 * The resulting string will be human-readable and in a format suitable for use
 * in SQL queries.  It won't have niceties such as "thousands separators"
 * though.
 */
template<typename T> inline std::string to_string(const T &obj);


/// Is @c value null?
template<typename TYPE> inline bool is_null(const TYPE &value)
{
  using traits = string_traits<TYPE>;
  if constexpr (traits::has_null) return traits::is_null(value);
  else return false;
}
//@}
} // namespace pqxx


#include "pqxx/internal/conversions.hxx"
#endif
