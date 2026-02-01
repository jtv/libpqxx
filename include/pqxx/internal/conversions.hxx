#if !defined(PQXX_H_CONVERSIONS)
#  define PQXX_H_CONVERSIONS

#  include <array>
#  include <concepts>
#  include <cstring>
#  include <map>
#  include <memory>
#  include <numeric>
#  include <optional>
#  include <span>
#  include <type_traits>
#  include <variant>

#  include "pqxx/encoding_group.hxx"
#  include "pqxx/strconv.hxx"


/* Internal helpers for string conversion, and conversion implementations.
 *
 * Do not include this header directly.  The libpqxx headers do it for you.
 */
namespace pqxx::internal
{
/// Convert a number in [0, 9] to its ASCII digit.
PQXX_PURE inline constexpr char number_to_digit(int i) noexcept
{
  PQXX_ASSUME(i >= 0);
  PQXX_ASSUME(i <= 9);
  return static_cast<char>(i + '0');
}


/// Compute numeric value of given textual digit (assuming that it is a digit).
/** The digit must be a base-10 digit.
 */
PQXX_PURE inline constexpr int digit_to_number(char c) noexcept
{
  PQXX_ASSUME(c >= '0');
  PQXX_ASSUME(c <= '9');
  return c - '0';
}


/// Summarize buffer overrun.
/** Don't worry about the exact parameter types: the sizes will be reasonably
 * small, and nonnegative.
 */
PQXX_LIBEXPORT std::string
state_buffer_overrun(int have_bytes, int need_bytes);


template<typename HAVE, typename NEED>
PQXX_INLINE_COV inline std::string
state_buffer_overrun(HAVE have_bytes, NEED need_bytes)
{
  return state_buffer_overrun(
    static_cast<int>(have_bytes), static_cast<int>(need_bytes));
}


/// Throw exception for attempt to convert SQL NULL to given type.
[[noreturn]] PQXX_COLD PQXX_LIBEXPORT void
throw_null_conversion(std::string const &type, sl);


/// Throw exception for attempt to convert SQL NULL to given type.
[[noreturn]] PQXX_COLD PQXX_LIBEXPORT void
throw_null_conversion(std::string_view type, sl);


/// Deliberately nonfunctional conversion traits for `char` types.
/** There are no string conversions for `char` and its signed and unsigned
 * variants.  Such a conversion would be dangerously ambiguous: should we treat
 * it as text, or as a small integer?  It'd be an open invitation for bugs.
 *
 * But the error message when you get this wrong is very cryptic.  So, we
 * derive dummy @ref string_traits implementations from this dummy type, and
 * ensure that the compiler disallows their use.  The compiler error message
 * will at least contain a hint of the root of the problem.
 */
template<pqxx::internal::char_type CHAR_TYPE>
struct disallowed_ambiguous_char_conversion
{
  static constexpr std::size_t
  size_buffer(CHAR_TYPE const &) noexcept = delete;

  static constexpr std::string_view
  to_buf(std::span<char>, CHAR_TYPE const &, ctx = {}) noexcept = delete;

  static CHAR_TYPE from_string(std::string_view, ctx = {}) = delete;
};


template<std::floating_point T>
PQXX_LIBEXPORT extern std::string to_string_float(T, ctx = {});


/// Generic implementation for `into_buf()`, on top of `to_buf()`.
template<typename T>
[[deprecated("into_buf() is no longer part of the string conversion API.")]]
inline char *
generic_into_buf(char *begin, char *end, T const &value, ctx c = {})
{
  zview text{to_buf({begin, end}, value, c)};
  auto const space{end - begin};
  // Include the trailing zero.
  auto const len = std::size(text) + 1;
  if (std::cmp_greater(len, space))
    throw conversion_overrun{
      std::format("Not enough buffer space to insert {}.", name_type<T>()) +
        state_buffer_overrun(space, len),
      c.loc};
  std::memmove(begin, std::data(text), len);
  return begin + len;
}


/// Generic implementation for `into_buf()`, on top of `to_buf()`.
template<typename T>
[[deprecated("into_buf() is no longer part of the string conversion API.")]]
inline std::size_t
generic_into_buf(std::span<char> buf, T const &value, ctx c = {})
{
  auto const begin{std::data(buf)};
  zview text{to_buf(buf, value, c)};
  auto const space{std::size(buf)};
  // Include the trailing zero.
  auto const len = std::size(text) + 1;
  if (std::cmp_greater(len, space))
    throw conversion_overrun{
      std::format("Not enough buffer space to insert {}.  ", name_type<T>()) +
        state_buffer_overrun(space, len),
      c.loc};
  std::memmove(begin, std::data(text), len);
  return len;
}


/// String traits for builtin floating-point types.
/** It _would_ make sense to define this directly as the definition for
 * `pqxx::string_traits<T>` where `T` is a `std::floating_point`.  However
 * Visual Studio 2022 does not seem to accept that syntax.
 *
 * So instead, we create a separate base class for `std::floating_point` types
 * and then derive specialisations of `pqxx::string_traits` from that.
 */
template<std::floating_point T> struct float_string_traits
{
  PQXX_LIBEXPORT static T from_string(std::string_view text, ctx = {});

  PQXX_LIBEXPORT static std::string_view
  to_buf(std::span<char> buf, T const &value, ctx c = {});

  // Return a nonnegative integral value's number of decimal digits.
  static constexpr std::size_t digits10(std::size_t value) noexcept
  {
    if (value < 10)
      return 1;
    else
      return 1 + digits10(value / 10);
  }

  PQXX_INLINE_COV static constexpr std::size_t size_buffer(T const &) noexcept
  {
    using lims = std::numeric_limits<T>;
    // See #328 for a detailed discussion on the maximum number of digits.
    //
    // In a nutshell: for the big cases, the scientific notation is always
    // the shortest one, and therefore the one that to_chars will pick.
    //
    // So... How long can the scientific notation get?  1 (for sign) + 1 (for
    // decimal point) + 1 (for 'e') + 1 (for exponent sign) + max_digits10 +
    // max number of digits in the exponent + 1 (terminating zero).
    //
    // What's the max number of digits in the exponent?  It's the max number of
    // digits out of the most negative exponent and the most positive one.
    //
    // The longest positive exponent is easy: 1 + ceil(log10(max_exponent10)).
    // (The extra 1 is because 10^n takes up 1 + n digits, not n.)
    //
    // The longest negative exponent is a bit harder: min_exponent10 gives us
    // the smallest power of 10 which a normalised version of T can represent.
    // But the smallest denormalised power of 10 that T can represent is
    // another max_digits10 powers of 10 below that.  It also needs room for a
    // a minus sign.
    //
    // All this stuff messes with my head a bit because it's on the order of
    // log10(log10(n)).  It's easy to get the number of logs wrong.
    auto const max_pos_exp{digits10(lims::max_exponent10)};
    // Really want std::abs(lims::min_exponent10), but MSVC 2017 apparently has
    // problems with std::abs.  So we use -lims::min_exponent10 instead.
    auto const max_neg_exp{
      digits10(lims::max_digits10 - lims::min_exponent10)};
    return 1 +                                    // Sign.
           1 +                                    // Decimal point.
           std::numeric_limits<T>::max_digits10 + // Mantissa digits.
           1 +                                    // Exponent "e".
           1 +                                    // Exponent sign.
           // Spell this weirdly to stop Windows compilers from reading this as
           // a call to their "max" macro when NOMINMAX is not defined.
           (std::max)(max_pos_exp, max_neg_exp); // Exponent digits.
  }
};

/// String traits for builtin integer types.
/** This does not cover `bool` or (unlike `std::integral`) the `char` types.
 *
 * Once upon a time this was a partial specialisation of `string_traits` for
 * the `pqxx::internal::integer` concept, but MSVC 2026 had a mysterious
 * problem with that (see #1160).  So, we had to revert to a much clunkier way
 * of creating those specialisations.
 */
template<pqxx::internal::integer T> struct integer_string_traits
{
  PQXX_LIBEXPORT static T from_string(std::string_view text, ctx = {});
  PQXX_LIBEXPORT static std::string_view
  to_buf(std::span<char> buf, T const &value, ctx c = {});

  PQXX_INLINE_ONLY static constexpr std::size_t size_buffer(T const &) noexcept
  {
    /** Includes a sign if needed; the number of base-10 digits which the type
     * can reliably represent; and the one extra base-10 digit which the type
     * can only partially represent.
     */
    return std::is_signed_v<T> + std::numeric_limits<T>::digits10 + 1;
  }
};
} // namespace pqxx::internal

namespace pqxx
{
/// The built-in arithmetic types do not have inherent null values.
/** Not-a-Number values (or NaNs for short) behave a lot like an SQL null, but
 * they are not nulls.  A non-null SQL float can be NaN.
 */
template<typename T>
  requires std::is_arithmetic_v<T>
struct nullness<T> final : no_null<T>
{};


template<pqxx::internal::integer T>
inline constexpr bool is_unquoted_safe<T>{true};
template<std::floating_point T>
inline constexpr bool is_unquoted_safe<T>{true};

#  define PQXX_SPECIALIZE_INT_TRAIT(typ)                                      \
    template<>                                                                \
    struct string_traits<typ> final                                           \
            : pqxx::internal::integer_string_traits<typ>                      \
    {}

PQXX_SPECIALIZE_INT_TRAIT(short);
PQXX_SPECIALIZE_INT_TRAIT(unsigned short);
PQXX_SPECIALIZE_INT_TRAIT(int);
PQXX_SPECIALIZE_INT_TRAIT(unsigned);
PQXX_SPECIALIZE_INT_TRAIT(long);
PQXX_SPECIALIZE_INT_TRAIT(unsigned long);
PQXX_SPECIALIZE_INT_TRAIT(long long);
PQXX_SPECIALIZE_INT_TRAIT(unsigned long long);

#  undef PQXX_SPECIALIZE_INT_TRAIT

template<>
struct string_traits<float> final : pqxx::internal::float_string_traits<float>
{};
template<>
struct string_traits<double> final
        : pqxx::internal::float_string_traits<double>
{};
template<>
struct string_traits<long double> final
        : pqxx::internal::float_string_traits<long double>
{};


template<> struct string_traits<bool> final
{
  PQXX_LIBEXPORT static bool from_string(std::string_view text, ctx c = {});

  static constexpr zview
  to_buf(std::span<char>, bool const &value, ctx = {}) noexcept
  {
    return value ? "true"_zv : "false"_zv;
  }

  static constexpr std::size_t size_buffer(bool const &) noexcept { return 5; }
};


template<> inline constexpr bool is_unquoted_safe<bool>{true};


template<typename T> struct nullness<std::optional<T>> final
{
  static constexpr bool has_null = true;
  /// Technically, you could have an optional of an always-null type.
  static constexpr bool always_null = pqxx::always_null<T>();
  [[nodiscard]] PQXX_PURE static constexpr bool
  is_null(std::optional<T> const &v) noexcept
  {
    return ((not v.has_value()) or pqxx::is_null(*v));
  }
  [[nodiscard]] PQXX_PURE static constexpr std::optional<T> null() noexcept
  {
    return {};
  }
};


template<typename T>
inline constexpr format param_format(std::optional<T> const &value)
{
  return param_format(*value);
}


template<typename T> struct string_traits<std::optional<T>> final
{
  static std::string_view
  to_buf(std::span<char> buf, std::optional<T> const &value, ctx c = {})
  {
    if (pqxx::is_null(value))
      return {};
    else
      return pqxx::to_buf(buf, *value, c);
  }

  static std::optional<T> from_string(std::string_view text, ctx c = {})
  {
    return std::optional<T>{std::in_place, pqxx::from_string<T>(text, c)};
  }

  static std::size_t size_buffer(std::optional<T> const &value) noexcept
  {
    if (pqxx::is_null(value))
      return 0;
    else
      return pqxx::size_buffer(value.value());
  }
};


template<typename T>
inline constexpr bool is_unquoted_safe<std::optional<T>>{is_unquoted_safe<T>};


template<typename... T> struct nullness<std::variant<T...>> final
{
  static constexpr bool has_null = (pqxx::has_null<T>() or ...);
  static constexpr bool always_null = (pqxx::always_null<T>() and ...);
  [[nodiscard]] PQXX_PURE static constexpr bool
  is_null(std::variant<T...> const &value) noexcept
  {
    return value.valueless_by_exception() or
           std::visit(
             [](auto const &i) noexcept {
               return nullness<std::remove_cvref_t<decltype(i)>>::is_null(i);
             },
             value);
  }

  // We don't support `null()` for `std::variant`.
  /** It would be technically possible to have a `null` in the case where just
   * one of the types has a null, but it gets complicated and arbitrary.
   */
  static constexpr std::variant<T...> null() = delete;
};


template<typename... T> struct string_traits<std::variant<T...>> final
{
  static std::string_view
  to_buf(std::span<char> buf, std::variant<T...> const &value, ctx c = {})
  {
    return std::visit(
      [buf, c](auto const &i) {
        using field_t = std::remove_cvref_t<decltype(i)>;
        return pqxx::to_buf<field_t>(buf, i, c);
      },
      value);
  }
  static std::size_t size_buffer(std::variant<T...> const &value) noexcept
  {
    if (pqxx::is_null(value))
      return 0;
    else
      return std::visit(
        [](auto const &i) noexcept { return pqxx::size_buffer(i); }, value);
  }

  /** There's no from_string for std::variant.  We could have one with a rule
   * like "pick the first type which fits the value," but we'd have to look
   * into how natural that API feels to users.
   */
  static std::variant<T...> from_string(std::string_view, ctx = {}) = delete;
};


template<typename... Args>
inline constexpr format param_format(std::variant<Args...> const &value)
{
  return std::visit([](auto &v) { return param_format(v); }, value);
}


template<typename... T>
inline constexpr bool is_unquoted_safe<std::variant<T...>>{
  (is_unquoted_safe<T> and ...)};


template<typename T>
inline T from_string(std::stringstream const &text, ctx c = {})
{
  return from_string<T>(text.str(), c);
}


template<> struct string_traits<std::nullptr_t> final
{
  [[deprecated("Do not convert nulls.")]] static constexpr zview
  to_buf(std::span<char>, std::nullptr_t const &, ctx = {}) noexcept
  {
    return {};
  }

  [[deprecated("Do not convert nulls.")]] static constexpr std::size_t
  size_buffer(std::nullptr_t = nullptr) noexcept
  {
    return 0;
  }
  static std::nullptr_t from_string(std::string_view, ctx = {}) = delete;
};


template<> struct string_traits<std::nullopt_t> final
{
  [[deprecated("Do not convert nulls.")]] static constexpr zview
  to_buf(char *, char *, std::nullopt_t const &) noexcept
  {
    return {};
  }

  [[deprecated("Do not convert nulls.")]] static constexpr std::size_t
  size_buffer(std::nullopt_t) noexcept
  {
    return 0;
  }
  static std::nullopt_t from_string(std::string_view, ctx = {}) = delete;
};


template<> struct string_traits<std::monostate> final
{
  [[deprecated("Do not convert nulls.")]] static constexpr zview
  to_buf(char *, char *, std::monostate const &) noexcept
  {
    return {};
  }

  [[deprecated("Do not convert nulls.")]] static constexpr std::size_t
  size_buffer(std::monostate) noexcept
  {
    return 0;
  }
  [[deprecated("Do not convert nulls.")]] static std::monostate
    from_string(std::string_view, ctx = {}) = delete;
};


template<> inline constexpr bool is_unquoted_safe<std::nullptr_t>{true};


template<> struct nullness<char const *> final
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = false;
  [[nodiscard]] PQXX_PURE PQXX_ZARGS static constexpr bool
  is_null(char const *t) noexcept
  {
    return t == nullptr;
  }
  [[nodiscard]] PQXX_PURE static constexpr char const *null() noexcept
  {
    return nullptr;
  }
};


/// String traits for C-style string ("pointer to char const").
/** This conversion is not bidirectional.  You can convert a C-style string to
 * an SQL string, but not the other way around.
 *
 * The reason for this is the terminating zero.  The incoming SQL string is a
 * `std::string_view`, which may or may not have a zero at the end.  (And
 * there's no reliable way of checking, since the next memory position may not
 * be a valid address.  Even if there happens to be a zero there, it isn't
 * necessarily part of the same block of mmory.)
 */
template<> struct string_traits<char const *> final
{
  static char const *from_string(std::string_view text, ctx = {}) = delete;

  constexpr PQXX_ZARGS static std::string_view
  to_buf(std::span<char>, char const *const &value, ctx = {}) noexcept
  {
    return value;
  }

  PQXX_ZARGS static constexpr std::size_t
  size_buffer(char const *value) noexcept
  {
    if (pqxx::is_null(value))
      return 0;
    else
      // std::char_traits::length() is like std::strlen(), but constexpr.
      return std::char_traits<char>::length(value);
  }
};


template<> struct nullness<char *> final
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = false;
  [[nodiscard]] PQXX_PURE static constexpr bool is_null(char *t) noexcept
  {
    return t == nullptr;
  }
  [[nodiscard]] PQXX_PURE static constexpr char *null() { return nullptr; }
};


/// String traits for non-const C-style string ("pointer to char").
/** This conversion is not bidirectional.  You can convert a `char *` to an
 * SQL string, but not vice versa.
 *
 * There are two reasons.  One is the fact that an SQL string arrives in the
 * form of a `std::string_view`; there is no guarantee of a trailing zero.
 *
 * The other reason is constness.  We can't give you a non-const pointer into
 * a string that was handed into the conversion as `const`.
 */
template<> struct string_traits<char *> final
{
  static std::string_view
  to_buf(std::span<char> buf, char *const &value, ctx c = {})
  {
    return pqxx::to_buf<char const *>(buf, value, c);
  }
  static std::size_t size_buffer(char *const &value) noexcept
  {
    if (pqxx::is_null(value))
      return 0;
    else
      return pqxx::size_buffer(const_cast<char const *>(value));
  }

  static char *from_string(std::string_view, ctx = {}) = delete;
};


template<std::size_t N> struct nullness<char[N]> final : no_null<char[N]>
{};


/// String traits for C-style string constant ("pointer to array of char").
/** @warning This assumes that every array-of-char is a C-style string literal.
 * So, it must include a trailing zero. and it must have static duration.
 */
template<std::size_t N> struct string_traits<char[N]> final
{
  static constexpr zview
  to_buf(std::span<char>, char const (&value)[N], ctx = {}) noexcept
  {
    return zview{value, N - 1};
  }

  static constexpr std::size_t size_buffer(char const (&)[N]) noexcept
  {
    return N;
  }

  /// Don't allow conversion to this type.
  static void from_string(std::string_view, ctx = {}) = delete;
};


template<> struct nullness<std::string> final : no_null<std::string>
{};


template<> struct string_traits<std::string> final
{
  PQXX_INLINE_ONLY static std::string
  from_string(std::string_view text, ctx = {})
  {
    return std::string{text};
  }

  PQXX_INLINE_ONLY static std::string_view
  to_buf(std::span<char>, std::string const &value, ctx = {})
  {
    return {std::data(value), std::size(value)};
  }

  PQXX_INLINE_ONLY static constexpr std::size_t
  size_buffer(std::string const &value) noexcept
  {
    return std::size(value);
  }
};


/// There's no real null value for `std::string_view`.
/** A `string_view` may have a null data pointer, which is analogous to a null
 * `char` pointer, but the standard does not really seem to guarantee that it
 * is distinct from other empty string views.
 */
template<> struct nullness<std::string_view> final : no_null<std::string_view>
{};


/// String traits for `string_view`.
/** @warning This conversion does not store the string's contents anywhere.
 * When you convert a string to a `std::string_view`, _do not access the
 * resulting view after the original string has been destroyed.  The contents
 * will no longer be valid, even though tests may not make this obvious.
 */
template<> struct string_traits<std::string_view> final
{
  PQXX_INLINE_ONLY static constexpr std::size_t
  size_buffer(std::string_view const &value) noexcept
  {
    return std::size(value);
  }

  PQXX_INLINE_ONLY static std::string_view
  to_buf(std::span<char>, std::string_view const &value, ctx = {})
  {
    return value;
  }

  PQXX_INLINE_ONLY static std::string_view
  from_string(std::string_view value, ctx = {})
  {
    return value;
  }
};


template<> struct nullness<zview> final : no_null<zview>
{};


/// String traits for `zview`.
template<> struct string_traits<zview> final
{
  PQXX_INLINE_ONLY static constexpr std::size_t
  size_buffer(std::string_view const &value) noexcept
  {
    return std::size(value);
  }

  static constexpr zview to_buf(std::span<char>, zview const &value, ctx = {})
  {
    return value;
  }

  /// Don't convert to this type.  There may not be a terminating zero.
  /** There is no valid way to figure out here whether there is a terminating
   * zero.  Even if there is one, that may just be the first byte of an
   * entirely separately allocated piece of memory.
   */
  static zview from_string(std::string_view, ctx = {}) = delete;
};


template<>
struct nullness<std::stringstream> final : no_null<std::stringstream>
{};


template<> struct string_traits<std::stringstream> final
{
  static std::size_t size_buffer(std::stringstream const &) = delete;

  static std::stringstream from_string(std::string_view text, ctx = {})
  {
    std::stringstream stream;
    stream.write(text.data(), std::streamsize(std::size(text)));
    return stream;
  }

  static std::string_view
  to_buf(std::span<char>, std::stringstream const &, ctx = {}) = delete;
};


template<>
struct nullness<std::nullptr_t> final : all_null<std::nullptr_t, nullptr>
{};

template<>
struct nullness<std::nullopt_t> final : all_null<std::nullopt_t, std::nullopt>
{};

template<>
struct nullness<std::monostate> final
        : all_null<std::monostate, std::monostate{}>
{};


template<typename T> struct nullness<std::unique_ptr<T>> final
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = false;
  [[nodiscard]] PQXX_PURE static constexpr bool
  is_null(std::unique_ptr<T> const &t) noexcept
  {
    return not t or pqxx::is_null(*t);
  }
  [[nodiscard]] PQXX_PURE static constexpr std::unique_ptr<T> null() noexcept
  {
    return {};
  }
};


template<typename T, typename... Args>
struct string_traits<std::unique_ptr<T, Args...>> final
{
  static std::unique_ptr<T> from_string(std::string_view text, ctx c = {})
  {
    return std::make_unique<T>(pqxx::from_string<T>(text, c));
  }

  static std::string_view to_buf(
    std::span<char> buf, std::unique_ptr<T, Args...> const &value, ctx c = {})
  {
    if (not value)
      internal::throw_null_conversion(name_type<std::unique_ptr<T>>(), c.loc);
    return pqxx::to_buf(buf, *value, c);
  }

  static std::size_t
  size_buffer(std::unique_ptr<T, Args...> const &value) noexcept
  {
    if (pqxx::is_null(value))
      return 0;
    else
      return pqxx::size_buffer(*value.get());
  }
};


template<typename T, typename... Args>
inline format param_format(std::unique_ptr<T, Args...> const &value)
{
  return param_format(*value);
}


template<typename T, typename... Args>
inline constexpr bool is_unquoted_safe<std::unique_ptr<T, Args...>>{
  is_unquoted_safe<T>};


template<typename T> struct nullness<std::shared_ptr<T>> final
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = false;
  [[nodiscard]] PQXX_PURE static constexpr bool
  is_null(std::shared_ptr<T> const &t) noexcept
  {
    return not t or pqxx::is_null(*t);
  }
  [[nodiscard]] PQXX_PURE static constexpr std::shared_ptr<T> null() noexcept
  {
    return {};
  }
};


template<typename T> struct string_traits<std::shared_ptr<T>> final
{
  static std::shared_ptr<T> from_string(std::string_view text, ctx c = {})
  {
    return std::make_shared<T>(pqxx::from_string<T>(text, c));
  }

  static std::string_view
  to_buf(std::span<char> buf, std::shared_ptr<T> const &value, ctx c = {})
  {
    if (not value)
      internal::throw_null_conversion(name_type<std::shared_ptr<T>>(), c.loc);
    return pqxx::to_buf(buf, *value, c);
  }
  static std::size_t size_buffer(std::shared_ptr<T> const &value) noexcept
  {
    if (pqxx::is_null(value))
      return 0;
    else
      return pqxx::size_buffer(*value);
  }
};


template<typename T> format param_format(std::shared_ptr<T> const &value)
{
  return param_format(*value);
}


template<typename T>
inline constexpr bool is_unquoted_safe<std::shared_ptr<T>>{
  is_unquoted_safe<T>};


template<binary DATA> struct nullness<DATA> final : no_null<DATA>
{};


template<binary DATA> struct string_traits<DATA> final
{
  static std::size_t size_buffer(DATA const &value) noexcept
  {
    return internal::size_esc_bin(std::size(value));
  }

  static std::string_view
  to_buf(std::span<char> buf, DATA const &value, ctx c = {})
  {
    // Budget for this type is precise.
    auto const budget{size_buffer(value)};
    if (std::cmp_less(std::size(buf), budget))
      throw conversion_overrun{
        "Not enough buffer space to escape binary data.", c.loc};
    internal::esc_bin(value, buf);
    // The budget included a terminating zero, which we do not include in the
    // view.
    return {std::data(buf), budget - 1};
  }

  /// Convert a string to binary data.
  static DATA from_string(std::string_view text, ctx c = {})
  {
    auto const size{pqxx::internal::size_unesc_bin(std::size(text))};
    DATA buf;
    if constexpr (requires { buf.resize(std::size_t{}); })
    {
      // Make `buf` allocate the number of bytes we need to store.
      buf.resize(size);
    }
    else
    {
      // There's no suitable `DATA::resize()`.  But perhaps the caller has
      // ensured that `DATA` is a type that inherently has the right size.
      // Might a `std::array<std::byte, ...>` for instance.
      if (std::size(buf) != size)
        throw conversion_error{
          std::format(
            "Can't convert binary data from SQL text to {}: I don't know how "
            "to "
            "resize that type.",
            name_type<DATA>()),
          c.loc};
    }

    pqxx::internal::unesc_bin(text, buf, c.loc);
    return buf;
  }
};
} // namespace pqxx


namespace pqxx::internal
{
template<nonbinary_range TYPE>
inline std::size_t array_into_buf(
  std::span<char> buf, TYPE const &value, std::size_t budget, ctx c = {});


/// Base class for `string_traits` specialisations for nonbinary ranges.
/** We use the same code for the `pqxx::array` traits, and I'm not sure how to
 * delegate directly to a specialisation for a broader concept.
 */
template<typename T> struct nonbinary_range_traits
{
  using elt_type = std::remove_cvref_t<value_type<T>>;
  using elt_traits = string_traits<elt_type>;
  static constexpr zview s_null{"NULL"};

  static std::size_t size_buffer(T const &value) noexcept
  {
    if constexpr (is_unquoted_safe<elt_type>)
      return 3 + std::accumulate(
                   std::begin(value), std::end(value), std::size_t{},
                   [](std::size_t acc, elt_type const &elt) {
                     // Budget for each element includes a terminating zero.
                     // We won't actually be wanting those, but don't subtract
                     // that one byte: we want room for a separator instead.
                     // However, std::size(s_null) doesn't account for the
                     // terminating zero, so add one to make s_null pay for its
                     // own separator.
                     return acc + (pqxx::is_null(elt) ?
                                     (std::size(s_null) + 1) :
                                     elt_traits::size_buffer(elt));
                   });
    else
      return 3 + std::accumulate(
                   std::begin(value), std::end(value), std::size_t{},
                   [](std::size_t acc, elt_type const &elt) {
                     // Opening and closing quotes, plus worst-case escaping,
                     // plus one byte for the separator.
                     std::size_t const elt_size{
                       pqxx::is_null(elt) ? std::size(s_null) :
                                            elt_traits::size_buffer(elt)};
                     return acc + 2 * elt_size + 3;
                   });
  }

  static std::string_view
  to_buf(std::span<char> buf, T const &value, ctx c = {})
  {
    auto const sz{
      pqxx::internal::array_into_buf(buf, value, size_buffer(value), c)};
    return {std::data(buf), sz};
  }
};
} // namespace pqxx::internal


namespace pqxx
{
template<nonbinary_range T> struct nullness<T> final : no_null<T>
{};


/// String traits for SQL arrays.
/** This is a very generic implementation,  It doesn't carry enough type
 * information about the container to support _parsing_ of a string into an
 * array of a more or less arbitrary C++ type, but it's handy for converting
 * various container types _to_ strings.
 */
template<nonbinary_range T>
struct string_traits<T> final : pqxx::internal::nonbinary_range_traits<T>
{};


/// We don't know how to pass array params in binary format, so pass as text.
template<nonbinary_range T> inline constexpr format param_format(T const &)
{
  return format::text;
}


/// A contiguous range of `std::byte` is a binary string; other ranges are not.
template<binary T> inline constexpr format param_format(T const &)
{
  return format::binary;
}


template<nonbinary_range T> inline constexpr bool is_sql_array<T>{true};


template<typename TYPE>
PQXX_INLINE_COV inline std::string to_string(TYPE const &value, ctx c)
{
  if (is_null(value))
    throw conversion_error{
      std::format("Attempt to convert null to a string.", name_type<TYPE>()),
      c.loc};

  if constexpr (pqxx::always_null<TYPE>())
  {
    // Have to separate out this case: some functions in the "regular" code
    // may not exist in the "always null" case.
    PQXX_UNREACHABLE;
    // C++23: The return may not be needed when std::unreachable is available.
    return {};
  }
  else
  {
    std::string buf;
    // We can't just reserve() space; modifying the terminating zero leads to
    // undefined behaviour.
    buf.resize(pqxx::size_buffer(value));
    std::size_t const stop{pqxx::into_buf(buf, value, c)};
    buf.resize(stop);
    return buf;
  }
}


template<> inline std::string to_string(float const &value, ctx c)
{
  return pqxx::internal::to_string_float(value, c);
}
template<> inline std::string to_string(double const &value, ctx c)
{
  return pqxx::internal::to_string_float(value, c);
}
template<> inline std::string to_string(long double const &value, ctx c)
{
  return pqxx::internal::to_string_float(value, c);
}
template<> inline std::string to_string(std::stringstream const &value, ctx)
{
  return value.str();
}


template<typename T>
inline void into_string(T const &value, std::string &out, ctx c = {})
{
  if (is_null(value))
    throw conversion_error{
      std::format("Attempt to convert null {} to a string.", name_type<T>()),
      c.loc};

  // We can't just reserve() data; modifying the terminating zero leads to
  // undefined behaviour.
  out.resize(size_buffer(value));
  auto const data{out.data()};
  auto const end{into_buf({data, std::size(out)}, value, c)};
  out.resize(static_cast<std::size_t>(end - data));
}
} // namespace pqxx
#endif
