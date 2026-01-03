/* String conversion definitions.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stringconv instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STRCONV
#define PQXX_H_STRCONV

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <algorithm>
#include <charconv>
#include <cstring>
#include <limits>
#include <ranges>
#include <sstream>
#include <stdexcept>
#include <typeinfo>

#include "pqxx/encoding_group.hxx"
#include "pqxx/except.hxx"
#include "pqxx/util.hxx"
#include "pqxx/zview.hxx"


namespace pqxx
{
/**
 * @defgroup stringconversion String conversion
 *
 * The PostgreSQL server accepts and represents data in string form.  It has
 * its own formats for various data types.  The string conversions define how
 * various C++ types translate to and from their respective PostgreSQL text
 * representations.
 *
 * Each conversion is defined by a specialisations of @c string_traits.  It
 * gets complicated if you want top performance, but until you do, all you
 * really need to care about when converting values between C++ in-memory
 * representations such as @c int and the postgres string representations is
 * the @c pqxx::to_string and @c pqxx::from_string functions.
 *
 * If you need to convert a type which is not supported out of the box, you'll
 * need to define your own specialisations for these templates, similar to the
 * ones defined here and in `pqxx/conversions.hxx`.  Any conversion code which
 * "sees" your specialisation will now support your conversion.  In particular,
 * you'll be able to read result fields into a variable of the new type.
 */
//@{


// TODO: Drop `ENABLE` in 9.x.
/// Traits describing a type's "null value," if any.
/** Some C++ types have a special value or state which correspond directly to
 * SQL's NULL.
 *
 * The @c nullness traits describe whether it exists, and whether a particular
 * value is null.
 *
 * @warning The `ENABLE` parameter is deprecated.  It exists to enable use of
 * `std::enable_if`.  As of C++20, use `requires` or a concept instead.
 */
template<typename TYPE, typename ENABLE = void> struct nullness final
{
  /// Does this type have a null value?
  static bool has_null;

  /// Is this type always null?
  /** This is only the case for a few types, such as `std::nullptr_t`. */
  static constexpr bool always_null = false;

  /// Is @c value a null?
  PQXX_PURE static bool is_null(TYPE const &value);

  /// Return a null value.
  /** Don't use this in generic code to compare a value and see whether it is
   * null.  Some types may have multiple null values which do not compare as
   * equal, or may define a null value which is not equal to anything including
   * itself, like in SQL.
   */
  [[nodiscard]] PQXX_PURE static TYPE null();
};


/// Nullness traits describing a type which does not have a null value.
template<typename TYPE> struct no_null
{
  /// Does @c TYPE have a "built-in null value"?
  /** For example, a pointer can equal @c nullptr, which makes a very natural
   * representation of an SQL null value.  For such types, the code sometimes
   * needs to make special allowances.
   *
   * for most types, such as @c int or @c std::string, there is no built-in
   * null.  If you want to represent an SQL null value for such a type, you
   * would have to wrap it in something that does have a null value.  For
   * example, you could use @c std::optional<int> for "either an @c int or a
   * null value."
   */
  static constexpr bool has_null = false;

  /// Are all values of this type null?
  /** There are a few special C++ types which are always null - mainly
   * @c std::nullptr_t.
   */
  static constexpr bool always_null = false;

  /// Does a given value correspond to an SQL null value?
  /** Most C++ types, such as @c int or @c std::string, have no inherent null
   * value.  But some types such as C-style string pointers do have a natural
   * equivalent to an SQL null.
   */
  [[nodiscard]] PQXX_PURE static constexpr bool is_null(TYPE const &) noexcept
  {
    return false;
  }
};


/// Nullness traits describing a type whose values are always null.
/** If you're writing a `nullness` specialisation for such a type, you can
 * optionally derive it from this type to save yourself a bit of code.
 *
 * The second template parameter specifies a compile-time constant null value
 * of this type.
 */
template<typename TYPE, TYPE null_value> struct all_null
{
  /// Does `TYPE` have a null value?
  static constexpr bool has_null = true;

  /// Is `TYPE` _always_ null?
  static constexpr bool always_null = true;

  /// Is the given `TYPE` value a null?
  [[nodiscard]] PQXX_PURE static constexpr bool is_null(TYPE const &) noexcept
  {
    return true;
  }

  /// Return a sample null value.  Requires `TYPE` to be default-constructible.
  [[nodiscard]] PQXX_PURE static constexpr TYPE null() { return null_value; }
};


/// Contextual parameters for string conversions implementations.
/** These are some "extra" items that libpqxx may be able to pass to a string
 * conversion operation in order to provide it with extra information.
 *
 * Yes, these could simply have been extra parameters in the conversion API,
 * but that makes it harder to add new features (which existing conversions
 * can often safely ignore) without breaking compatibility.
 *
 * This type was introduced in libpqxx 8.  The older conversion API does not
 * know about it, so older code will just have a default-constructed context.
 */
struct conversion_context final
{
  /// Encoding group describing the client text encoding.
  /** This will not tell you what the exact _encoding_ is.  All libpqxx cares
   * about is how to parse text in a given encoding, so that it can reliably
   * detect escape characters, closing quotes, and so on.  It has no need to
   * "understand" the text beyond that.
   */
  encoding_group enc = encoding_group::unknown;

  /// A `std::source_location` for the call.
  /** When libpqxx throws an error, it will generally try to include this
   * information to help you debug the problem.  However this is not always
   * possible.
   *
   * Generally it will be helpful to pass the location where the client called
   * into libpqxx.  However if you don't pass a source location, this will use
   * the location in the source code where you created this `ctx`.
   */
  sl loc = sl::current();

  constexpr conversion_context(sl lc = sl::current()) : loc{lc} {}

  explicit constexpr conversion_context(
    encoding_group e, sl l = sl::current()) :
          enc{e}, loc{l}
  {}
};


/// Convenience alias: `const` reference to a @ref pqxx::conversion_context.
/** Because we need to squeeze this type into lots of function signatures, it
 * helps to have a very terse name for it.
 */
using ctx = conversion_context const &;


/// Traits class for use in string conversions.
/** Specialize this template for a type for which you wish to add to_string
 * and from_string support.
 *
 * String conversions are not meant to work for nulls.  Check for null before
 * converting a value of @c TYPE to a string, or vice versa, and handle them
 * separately.
 */
template<typename TYPE> struct string_traits final
{
  /// Return a @c string_view representing `value`.
  /** Produces a view on a PostgreSQL string representation for @c value.
   *
   * @warning A null value has no string representation.  Do not pass a null.
   *
   * Uses `buf` to store string contents, _if needed._  The returned
   * `string view` may point somewhere inside that buffer, or to a
   * compile-time constant, or just directly to `value`.  Even if the string
   * does live in `buf`, it may not start at the exact _beginning_ of `buf`.
   *
   * The resulting view stays valid for as long as both the buffer space to
   * which `buf` points, and `value`, remain accessible and unmodified.
   *
   * @throws pqxx::conversion_overrun if `buf` is not large enough.  For
   * maximum performance, this is a conservative estimate.  It may complain
   * about a buffer which is actually large enough for your value, if an exact
   * check would be too expensive.
   *
   * If there is no support for converting this type to an SQL string, simply
   * leave this function out of the struct.
   */
  [[nodiscard]] static inline std::string_view
  to_buf(std::span<char> buf, TYPE const &value, ctx = {});

  /// Parse a string representation of a @c TYPE value.
  /** Throws @c conversion_error if @c value does not meet the expected format
   * for a value of this type.
   *
   * @warning A null value has no string representation.  Do not parse a null.
   *
   * @warning If you convert a string to `std::string_view`, you're basically
   * just getting a pointer into the original buffer.  So, the `string_view`
   * will become invalid when the original string's lifetime ends, or when it
   * gets overwritten.  Do not access the `string_view` after that!
   *
   * If there is no support for converting from an SQL string to this type,
   * simply leave this function out of the struct.
   */
  [[nodiscard]] static inline TYPE
  from_string(std::string_view text, ctx = {});

  /// Estimate how much buffer space is needed to represent value.
  /** The estimate may be a little pessimistic, if it saves time.
   */
  [[nodiscard]] static inline std::size_t
  size_buffer(TYPE const &value) noexcept;

  // TODO: Move is_unquoted_safe into the traits after all?
};


// C++26: Use "=delete" with reason.
/// String traits for a forbidden type conversion.
/** If you have a C++ type for which you explicitly wish to forbid SQL
 * conversion, you can derive a @ref pqxx::string_traits specialisation for
 * that type from this struct.  Any attempt to convert the type will then fail
 * to build, and produce an error referring to this type.  It may help debug
 * build errors.
 */
template<typename TYPE> struct forbidden_conversion
{
  [[noreturn]] static std::string_view
  to_buf(std::span<char>, TYPE const &, ctx = {}) = delete;
  [[noreturn]] static TYPE from_string(std::string_view, ctx = {}) = delete;
  [[noreturn]] static std::size_t size_buffer(TYPE const &) noexcept = delete;
};


/// You cannot convert a `char` to/from SQL.
/** Converting this type may seem simple enough, but it's ambiguous: Did you
 * mean the `char` value as a small integer?  If so, did you mean it to be
 * signed or unsigned?  (The C++ Standard allows the system to implement `char`
 * as either a signed type or an unsigned type.)  Or were you thinking of a
 * single-character string (and if so, using what encoding)?  Or perhaps it's
 * just a raw byte value?
 *
 * If you meant it as an integer, use an appropriate integral type such as
 * `int` or `short` or `unsigned int` etc.
 *
 * If you wanted a single-character string, use `std::string_view` (or a
 * similar type such as `std::string`).
 *
 * Or if you had a raw byte in mind, try `pqxx::bytes_view` instead.
 */
template<> struct string_traits<char> final : forbidden_conversion<char>
{};


/// You cannot convert an `unsigned char` to/from SQL.
/** Converting this type may seem simple enough, but it's ambiguous: Did you
 * mean the `char` value as a small integer?  Or were you thinking of a
 * single-character string (and if so, using what encoding)?  Or perhaps it's
 * just a raw byte value?
 *
 * If you meant it as an integer, use an appropriate integral type such as
 * `int` or `short` or `unsigned int` etc.
 *
 * If you wanted a single-character string, use `std::string_view` (or a
 * similar type such as `std::string`).
 *
 * Or if you had a raw byte in mind, try `pqxx::bytes_view` instead.
 */
template<>
struct string_traits<unsigned char> final : forbidden_conversion<unsigned char>
{};


/// You cannot convert a `signed char` to/from SQL.
/** Converting this type may seem simple enough, but it's ambiguous: Did you
 * mean the value as a small integer?  Or were you thinking of a
 * single-character string (and if so, in what encoding)?  Or perhaps it's just
 * a raw byte value?
 *
 * If you meant it as an integer, use an appropriate integral type such as
 * `int` or `short` etc.
 *
 * If you wanted a single-character string, use `std::string_view` (or a
 * similar type such as `std::string`).
 *
 * Or if you had a raw byte in mind, try `pqxx::bytes_view` instead.
 */
template<>
struct string_traits<signed char> final : forbidden_conversion<signed char>
{};


/// You cannot convert a `std::byte` to/from SQL.
/** To convert a raw byte value, use a `bytes_view`.
 *
 * For example, to convert a byte `b` from C++ to SQL, convert the value
 * `pqxx::bytes_view{&b, 1}` instead.
 */
template<>
struct string_traits<std::byte> final : forbidden_conversion<std::byte>
{};


/// Nullness: Enums do not have an inherent null value.
template<enum_type ENUM> struct nullness<ENUM> final : no_null<ENUM>
{};
} // namespace pqxx

namespace pqxx::internal
{
/// Signature for string_traits<TYPE>::to_buf() in libpqxx 7.
template<typename TYPE>
concept to_buf_7 =
  requires(zview out, char *begin, char *end, TYPE const &value) {
    out = string_traits<TYPE>::to_buf(begin, end, value);
  };

/// Signature for string_traits<TYPE>::to_buf() in libpqxx 8.
template<typename TYPE>
concept to_buf_8 = requires(
  std::string_view out, std::span<char> buf, TYPE const &value, ctx c) {
  out = string_traits<TYPE>::to_buf(buf, value, c);
  out = string_traits<TYPE>::to_buf(buf, value);
};


/// Signature for string_traits<TYPE>::from_string() in libpqxx 8.
template<typename TYPE>
concept from_string_8 = requires(TYPE out, std::string_view text, ctx c) {
  out = string_traits<TYPE>::from_string(text, c);
  out = string_traits<TYPE>::from_string(text);
};

/// Signature for string_traits<TYPE>::from_string() in libpqxx 7.
/** This is actually identical to the new format, except the latter accepts an
 * optional @ref conversion_context argument.
 */
template<typename TYPE>
concept from_string_7 = requires(TYPE out, std::string_view text) {
  out = string_traits<TYPE>::from_string(text);
};
} // namespace pqxx::internal


namespace pqxx
{
/// Estimate how much buffer space is needed to represent values as a string.
/** The estimate may be a little pessimistic, if it saves time.
 */
template<typename... TYPE>
[[nodiscard]] inline constexpr std::size_t
size_buffer(TYPE const &...value) noexcept
{
  return (string_traits<std::remove_cvref_t<TYPE>>::size_buffer(value) + ...);
}


/// Is the libpqxx 8 version of `to_buf()` supported for `TYPE`?
/** The @ref pqxx::string_traits specialisation for `TYPE` must support either
 * the 7.x `to_buf` API or the 8.x one.
 *
 * The real reason this function exists is to make the compiler check for API
 * compliance... and if necessary, give a _helpful_ error message.  For that, a
 * `requires` is better than a `static_assert()`.  However, we can't use it in
 * the calling function's signature, since it requires a definition for the
 * relevant specialisation of @ref pqxx::string_traits _before_ the calling
 * function's.  That complicates things.
 */
template<typename TYPE>
  requires(pqxx::internal::to_buf_7<TYPE> or pqxx::internal::to_buf_8<TYPE>)
consteval bool supports_to_buf_8()
{
  return requires { requires pqxx::internal::to_buf_8<TYPE>; };
}


/// Represent `value` as SQL text, optionally using `buf` as storage.
/** This calls string_traits<TYPE>::to_buf(), but bridges some API version
 * differences.
 */
template<typename TYPE>
[[nodiscard]] inline std::string_view
to_buf(std::span<char> buf, TYPE const &value, ctx c = {})
{
  using traits = string_traits<TYPE>;
  if constexpr (supports_to_buf_8<TYPE>())
  {
    return traits::to_buf(buf, value, c);
  }
  else
  {
    auto const begin{std::data(buf)}, end{begin + std::size(buf)};
    return traits::to_buf(begin, end, value);
  }
}


/// Write an SQL representation of `value` into `buf`.
/** This calls @ref string_traits<TYPE>::to_buf(), but bridges some API version
 * differences.
 *
 * @return The number of SQL text bytes written into `buf`.  There is no
 * terminating zero.
 */
template<typename TYPE>
[[nodiscard]] inline std::size_t
into_buf(std::span<char> buf, TYPE const &value, ctx c = {})
{
  auto const data{std::data(buf)};
  std::string_view out{to_buf(buf, value, c)};
  auto const sz{std::size(out)};

  // Be careful to use a memory "move," not a "copy."  Source and destination
  // may overlap (or be identical).
  if (not std::empty(out)) [[likely]]
  {
    if (std::cmp_greater(std::size(out), std::size(buf)))
      throw conversion_overrun{
        std::format(
          "Buffer too small to convert {} value to string (needs a {}-byte "
          "buffer).",
          name_type<TYPE>(), std::size(out)),
        c.loc};
    std::memmove(data, std::data(out), sz);
  }

  return sz;
}


/// Is the libpqxx 8 version of `from_string()` supported for `TYPE`?
/** The @ref pqxx::string_traits specialisation for `TYPE` must support either
 * the 7.x `to_buf` API or the 8.x one.
 */
template<typename TYPE>
  requires(
    pqxx::internal::from_string_7<TYPE> or pqxx::internal::from_string_8<TYPE>)
consteval bool supports_from_string_8()
{
  return requires { requires pqxx::internal::from_string_8<TYPE>; };
}


/// Parse a value in postgres' text format as a TYPE.
/** If the form of the value found in the string does not match the expected
 * type, e.g. if a decimal point is found when converting to an integer type,
 * the conversion fails.  Overflows (e.g. converting "9999999999" to a 16-bit
 * C++ type) are also treated as errors.  If in some cases this behaviour
 * should be inappropriate, convert to something bigger such as @c long @c int
 * first and then truncate the resulting value.
 *
 * Only the simplest possible conversions are supported.  Fancy features like
 * hexadecimal or octal, spurious signs, or exponent notation won't work.
 * Whitespace is not stripped away.  Only the kinds of strings that come out of
 * PostgreSQL and out of to_string() can be converted.
 */
template<typename TYPE>
[[nodiscard]] inline TYPE from_string(std::string_view text, ctx c = {})
{
  if constexpr (supports_from_string_8<TYPE>())
    return string_traits<TYPE>::from_string(text, c);
  else
    return string_traits<TYPE>::from_string(text);
}


/// "Convert" a std::string_view to a std::string_view.
/** Just returns its input.
 *
 * @warning Of course the result is only valid for as long as the original
 * string remains valid!  Never access the string referenced by the return
 * value after the original has been destroyed.
 */
template<>
[[nodiscard]] inline std::string_view from_string(std::string_view text, ctx)
{
  return text;
}


/// Attempt to convert postgres-generated string to given built-in object.
/** This is like the single-argument form of the function, except instead of
 * returning the value, it sets @c value.
 *
 * You may find this more convenient in that it infers the type you want from
 * the argument you pass.  But there are disadvantages: it requires an
 * assignment operator, and it may be less efficient.
 */
template<typename T>
inline void from_string(std::string_view text, T &value, ctx c = {})
{
  value = from_string<T>(text, c);
}


/// Convert a value to a readable string that PostgreSQL will understand.
/** The conversion does no special formatting, and ignores any locale settings.
 * The resulting string will be human-readable and in a format suitable for use
 * in SQL queries.  It won't have niceties such as "thousands separators"
 * though.
 */
template<typename TYPE>
inline std::string to_string(TYPE const &value, ctx c = {});
} // namespace pqxx


namespace pqxx::internal
{
/// Helper class for defining enum conversions.
/** The conversion will convert enum values to numeric strings, and vice versa.
 *
 * @warning These numeric strings do not actually help you with the database,
 * where enum values are textual strings.  Once we have reflection in C++26, it
 * should become easier to provide real support.
 *
 * To define a string conversion for an enum type, derive a @c string_traits
 * specialisation for the enum from this struct.
 *
 * There's usually an easier way though: the @ref PQXX_DECLARE_ENUM_CONVERSION
 * macro.  Use `enum_traits` manually only if you need to customise your
 * traits type in more detail.
 */
template<enum_type ENUM> struct enum_traits
{
  using impl_type = std::underlying_type_t<ENUM>;

  [[nodiscard]] static constexpr std::string_view
  to_buf(std::span<char> buf, ENUM const &value, ctx c = {})
  {
    return pqxx::to_buf(buf, to_underlying(value), c);
  }

  [[nodiscard]] static ENUM from_string(std::string_view text, ctx c = {})
  {
    return static_cast<ENUM>(pqxx::from_string<impl_type>(text, c));
  }

  [[nodiscard]] static constexpr std::size_t
  size_buffer(ENUM const &value) noexcept
  {
    return pqxx::size_buffer(to_underlying(value));
  }

private:
  // C++23: Replace with std::to_underlying.
  static constexpr impl_type to_underlying(ENUM const &value) noexcept
  {
    return static_cast<impl_type>(value);
  }
};
} // namespace pqxx::internal


/// Macro: Define a string conversion for an enum type.
/** This specialises the @ref pqxx::string_traits and @ref pqxx::name_type
 * templates, so use it in the @ref pqxx namespace.
 *
 * For example:
 *
 *      #include <iostream>
 *      #include <pqxx/strconv>
 *      enum X { xa, xb };
 *      namespace pqxx { PQXX_DECLARE_ENUM_CONVERSION(x); }
 *      int main() { std::cout << pqxx::to_string(xa) << std::endl; }
 */
#define PQXX_DECLARE_ENUM_CONVERSION(ENUM)                                    \
  template<>                                                                  \
  [[maybe_unused, deprecated("Use name_type() instead of type_name.")]]       \
  constexpr inline std::string_view type_name<ENUM>{#ENUM};                   \
  template<> inline constexpr std::string_view name_type<ENUM>() noexcept     \
  {                                                                           \
    return #ENUM;                                                             \
  }                                                                           \
  template<>                                                                  \
  struct string_traits<ENUM> final : pqxx::internal::enum_traits<ENUM>        \
  {}


namespace pqxx
{
/// Convert multiple values to strings inside a single buffer.
/** There must be enough room for all values, or this will throw
 * @c conversion_overrun.  You can obtain a conservative estimate of the buffer
 * space required by calling @c size_buffer() on the values.
 *
 * The @c std::string_view results may point into the buffer, so don't assume
 * that they will remain valid after you destruct or move the buffer.
 */
template<typename... TYPE>
[[nodiscard, deprecated("Pass span and string_view.")]]
inline std::vector<std::string_view>
to_buf(char *begin, char const *end, TYPE... value)
{
  assert(begin <= end);
  // We can't construct the span as {begin, end} because end points to const.
  // Works fine on gcc 13, but clang 18 vomits huge cryptic errors.
  std::span<char> buf{
    begin, check_cast<std::size_t>(
             end - begin, "string_view too large.", sl::current())};
  std::size_t here{0u};
  return {[&here, buf](auto v) {
    auto start{here};
    here += pqxx::into_buf(buf.subspan(start), v);
    assert(start < here);
    assert(here <= std::size(buf));
    // C++26: Use buf.at().
    assert(buf[here - 1] == '\0');
    // Exclude the trailing zero out of the string_view.
    auto len{here - start - 1};
    return std::string_view{std::data(buf) + start, len};
  }(value)...};
}


/// Convert multiple values to strings inside a single buffer.
/** There must be enough room for all values, or this will throw
 * @c conversion_overrun.  You can obtain a conservative estimate of the buffer
 * space required by calling @c size_buffer() on the values.
 *
 * The @c std::string_view results may point into the buffer, so don't assume
 * that they will remain valid after you destruct or move the buffer.
 */
template<typename... TYPE>
inline std::vector<std::string_view>
to_buf_multi(ctx c, std::span<char> buf, TYPE... value)
{
  // TODO: Would it be worth merging consecutive identical strings?
  std::size_t here{0u};
  return {[&here, buf, &c](auto v) {
    auto start{here};
    here += pqxx::into_buf(buf.subspan(start), v, c);
    assert(start < here);
    assert(here <= std::size(buf));
    // C++26: Use buf.at().
    auto const len{here - start};
    return std::string_view{std::data(buf) + start, len};
  }(value)...};
}


/// Convert multiple values to strings inside a single buffer.
/** There must be enough room for all values, or this will throw
 * @c conversion_overrun.  You can obtain a conservative estimate of the buffer
 * space required by calling @c size_buffer() on the values.
 *
 * The @c std::string_view results may point into the buffer, so don't assume
 * that they will remain valid after you destruct or move the buffer.
 *
 * @warning This version of the function does not take a
 * @ref pqxx::conversion_context argument.  Prefer the version that does take
 * one, and pass (insofar as possible) a @ref pqxx::encoding_group and a
 * `std::source_location`.
 */
template<typename... TYPE>
inline std::vector<std::string_view>
to_buf_multi(std::span<char> buf, TYPE... value)
{
  // TODO: Would it be worth merging consecutive identical strings?
  std::size_t here{0u};
  conversion_context c{};
  return {[&here, buf, &c](auto v) {
    auto start{here};
    here += pqxx::into_buf(buf.subspan(start), v, c);
    assert(start < here);
    assert(here <= std::size(buf));
    // C++26: Use buf.at().
    auto const len{here - start};
    return std::string_view{std::data(buf) + start, len};
  }(value)...};
}


/// Convert a value to a readable string that PostgreSQL will understand.
/** This variant of to_string can sometimes save a bit of time in loops, by
 * re-using a std::string for multiple conversions.
 */
template<typename TYPE>
inline void into_string(TYPE const &value, std::string &out);


/// Does `TYPE` have one or more inherent null values?
/** Some types, such as pointers, have natural null values built in.  Most
 * types do not: an integer is never null (even if it may be zero), a C++
 * string is never null (even if it may be empty), and so on.
 *
 * This is different from an SQL field _containing_ a null value, but in
 * libpqxx you can treat them pretty much the same.
 */
template<typename TYPE> [[nodiscard]] inline constexpr bool has_null() noexcept
{
  using base_type = std::remove_cvref_t<TYPE>;
  return nullness<base_type>::has_null;
}


/// Is a value of `TYPE` always null?
/** This is the case for some special types, such as `std::nullopt_t`.
 */
template<typename TYPE>
[[nodiscard]] inline constexpr bool always_null() noexcept
{
  using base_type = std::remove_cvref_t<TYPE>;
  return nullness<base_type>::always_null;
}


/// Is `value` a null?
template<typename TYPE>
[[nodiscard]] inline constexpr bool is_null(TYPE const &value) noexcept
{
  using base_type = std::remove_cvref_t<TYPE>;
  if constexpr (always_null<TYPE>())
    return true;
  else
    return nullness<base_type>::is_null(value);
}


/// Return a null value of `TYPE`.
/** Only available for types that _have_ a null value.
 *
 * There may be multiple null values, or even ones that look the same but
 * which don't compare as equal, as is the case in SQL.
 */
template<typename TYPE>
[[nodiscard]] inline constexpr TYPE make_null()
  requires(pqxx::has_null<TYPE>())
{
  using base_type = std::remove_cvref_t<TYPE>;
  return nullness<base_type>::null();
}


/// Does this type translate to an SQL array?
/** Specialisations may override this to be true for container types.
 *
 * This may not always be a black-and-white choice.  For instance, a
 * @c std::string is a container, but normally it translates to an SQL string,
 * not an SQL array.
 */
template<typename TYPE> inline constexpr bool is_sql_array{false};


/// Can we use this type in arrays and composite types without quoting them?
/** Define this as @c true only if values of @c TYPE can never contain any
 * special characters that might need escaping or confuse the parsing of array
 * or composite * types, such as commas, quotes, parentheses, braces, newlines,
 * and so on.
 *
 * When converting a value of such a type to a string in an array or a field in
 * a composite type, we do not need to add quotes, nor escape any special
 * characters.
 *
 * This is just an optimisation, so it defaults to @c false to err on the side
 * of slow correctness.
 */
template<typename TYPE> inline constexpr bool is_unquoted_safe{false};


/// Element separator between SQL array elements of this type.
template<typename T> inline constexpr char array_separator{','};


/// What's the preferred format for passing non-null parameters of this type?
/** This affects how we pass parameters of @c TYPE when calling parameterised
 * statements or prepared statements.
 *
 * Generally we pass parameters in text format, but binary strings are the
 * exception.  We also pass nulls in binary format, so this function need not
 * handle null values.
 */
template<typename TYPE> inline constexpr format param_format(TYPE const &)
{
  return format::text;
}
//@}
} // namespace pqxx


#include "pqxx/internal/conversions.hxx"
#endif
