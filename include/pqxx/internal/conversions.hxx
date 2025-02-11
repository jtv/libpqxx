#include <array>
#include <concepts>
#include <cstring>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

#include "pqxx/types.hxx"
#include "pqxx/util.hxx"


/* Internal helpers for string conversion, and conversion implementations.
 *
 * Do not include this header directly.  The libpqxx headers do it for you.
 */
namespace pqxx::internal
{
/// Convert a number in [0, 9] to its ASCII digit.
inline constexpr char number_to_digit(int i) noexcept
{
  return static_cast<char>(i + '0');
}


/// Compute numeric value of given textual digit (assuming that it is a digit).
constexpr int digit_to_number(char c) noexcept
{
  return c - '0';
}


/// Summarize buffer overrun.
/** Don't worry about the exact parameter types: the sizes will be reasonably
 * small, and nonnegative.
 */
std::string PQXX_LIBEXPORT
state_buffer_overrun(int have_bytes, int need_bytes);


template<typename HAVE, typename NEED>
inline std::string state_buffer_overrun(HAVE have_bytes, NEED need_bytes)
{
  return state_buffer_overrun(
    static_cast<int>(have_bytes), static_cast<int>(need_bytes));
}


/// Throw exception for attempt to convert SQL NULL to given type.
[[noreturn]] PQXX_LIBEXPORT PQXX_COLD void
throw_null_conversion(std::string const &type);


/// Throw exception for attempt to convert SQL NULL to given type.
[[noreturn]] PQXX_LIBEXPORT PQXX_COLD void
throw_null_conversion(std::string_view type);


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
  static constexpr bool converts_to_string{false};
  static constexpr bool converts_from_string{false};
  static char *into_buf(char *, char *, CHAR_TYPE) = delete;
  static constexpr zview
  to_buf(char *, char *, CHAR_TYPE const &) noexcept = delete;

  static constexpr std::size_t
  size_buffer(CHAR_TYPE const &) noexcept = delete;
  static CHAR_TYPE from_string(std::string_view) = delete;
};


template<typename T> PQXX_LIBEXPORT extern std::string to_string_float(T);


/// Generic implementation for into_buf, on top of to_buf.
template<typename T>
inline char *generic_into_buf(char *begin, char *end, T const &value)
{
  zview const text{string_traits<T>::to_buf(begin, end, value)};
  auto const space{end - begin};
  // Include the trailing zero.
  auto const len = std::size(text) + 1;
  if (std::cmp_greater(len, space))
    throw conversion_overrun{
      "Not enough buffer space to insert " + type_name<T> + ".  " +
      state_buffer_overrun(space, len)};
  std::memmove(begin, text.data(), len);
  return begin + len;
}


/// String traits for builtin floating-point types.
/** It _would_ make sense to define this directly as the definition for
 * `pqxx::string_traits<T>` where `T` is a `std::floating_point`.  However
 * Viual Studio 2022 does not seem to accept that syntax.
 *
 * So instead, we create a separate base class for `std::floating_point` types
 * and then derive specialisatinos of `pqxx::string_traits` from that.
 */
template<std::floating_point T> struct float_string_traits
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{true};

  static PQXX_LIBEXPORT T from_string(std::string_view text);

  static PQXX_LIBEXPORT pqxx::zview
  to_buf(char *begin, char *end, T const &value);

  static PQXX_LIBEXPORT char *into_buf(char *begin, char *end, T const &value);

  // Return a nonnegative integral value's number of decimal digits.
  static constexpr std::size_t digits10(std::size_t value) noexcept
  {
    if (value < 10)
      return 1;
    else
      return 1 + digits10(value / 10);
  }

  static constexpr std::size_t size_buffer(T const &) noexcept
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
    // another max_digits10 powers of 10 below that.
    // needs a minus sign.
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
           (std::max)(max_pos_exp, max_neg_exp) + // Exponent digits.
           1;                                     // Terminating zero.
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
struct nullness<T, std::enable_if_t<std::is_arithmetic_v<T>>> : no_null<T>
{};


/// String traits for builtin integer types.
/** This does not cover `bool` or (unlike `std::integral`) the `char` types.
 */
template<pqxx::internal::integer T> struct string_traits<T>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{true};
  static PQXX_LIBEXPORT T from_string(std::string_view text);
  static PQXX_LIBEXPORT zview to_buf(char *begin, char *end, T const &value);
  static PQXX_LIBEXPORT char *into_buf(char *begin, char *end, T const &value);

  static constexpr std::size_t size_buffer(T const &) noexcept
  {
    /** Includes a sign if needed; the number of base-10 digits which the type
     * can reliably represent; the one extra base-10 digit which the type can
     * only partially represent; and the terminating zero.
     */
    return std::is_signed_v<T> + std::numeric_limits<T>::digits10 + 1 + 1;
  }
};


template<pqxx::internal::integer T>
inline constexpr bool is_unquoted_safe<T>{true};
template<std::floating_point T>
inline constexpr bool is_unquoted_safe<T>{true};


template<>
struct string_traits<float> : pqxx::internal::float_string_traits<float>
{};
template<>
struct string_traits<double> : pqxx::internal::float_string_traits<double>
{};
template<>
struct string_traits<long double>
        : pqxx::internal::float_string_traits<long double>
{};


template<> struct string_traits<bool>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{true};

  static PQXX_LIBEXPORT bool from_string(std::string_view text);

  static constexpr zview to_buf(char *, char *, bool const &value) noexcept
  {
    return value ? "true"_zv : "false"_zv;
  }

  static char *into_buf(char *begin, char *end, bool const &value)
  {
    return pqxx::internal::generic_into_buf(begin, end, value);
  }

  static constexpr std::size_t size_buffer(bool const &) noexcept { return 6; }
};


template<> inline constexpr bool is_unquoted_safe<bool>{true};


template<typename T> struct nullness<std::optional<T>>
{
  static constexpr bool has_null = true;
  /// Technically, you could have an optional of an always-null type.
  static constexpr bool always_null = nullness<T>::always_null;
  static constexpr bool is_null(std::optional<T> const &v) noexcept
  {
    return ((not v.has_value()) or pqxx::is_null(*v));
  }
  static constexpr std::optional<T> null() { return {}; }
};


template<typename T>
inline constexpr format param_format(std::optional<T> const &value)
{
  return param_format(*value);
}


template<typename T> struct string_traits<std::optional<T>>
{
  static constexpr bool converts_to_string{
    string_traits<T>::converts_to_string};
  static constexpr bool converts_from_string{
    string_traits<T>::converts_from_string};

  static char *into_buf(char *begin, char *end, std::optional<T> const &value)
  {
    return string_traits<T>::into_buf(begin, end, *value);
  }

  static zview to_buf(char *begin, char *end, std::optional<T> const &value)
  {
    if (pqxx::is_null(value))
      return {};
    else
      return string_traits<T>::to_buf(begin, end, *value);
  }

  static std::optional<T> from_string(std::string_view text)
  {
    return std::optional<T>{
      std::in_place, string_traits<T>::from_string(text)};
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


template<typename... T> struct nullness<std::variant<T...>>
{
  static constexpr bool has_null = (nullness<T>::has_null or ...);
  static constexpr bool always_null = (nullness<T>::always_null and ...);
  static constexpr bool is_null(std::variant<T...> const &value) noexcept
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


template<typename... T> struct string_traits<std::variant<T...>>
{
  static constexpr bool converts_to_string{
    (string_traits<T>::converts_to_string and ...)};

  static char *
  into_buf(char *begin, char *end, std::variant<T...> const &value)
  {
    return std::visit(
      [begin, end](auto const &i) {
        return string_traits<std::remove_cvref_t<decltype(i)>>::into_buf(
          begin, end, i);
      },
      value);
  }
  static zview to_buf(char *begin, char *end, std::variant<T...> const &value)
  {
    return std::visit(
      [begin, end](auto const &i) {
        return string_traits<std::remove_cvref_t<decltype(i)>>::to_buf(
          begin, end, i);
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
  static std::variant<T...> from_string(std::string_view) = delete;
};


template<typename... Args>
inline constexpr format param_format(std::variant<Args...> const &value)
{
  return std::visit([](auto &v) { return param_format(v); }, value);
}


template<typename... T>
inline constexpr bool is_unquoted_safe<std::variant<T...>>{
  (is_unquoted_safe<T> and ...)};


template<typename T> inline T from_string(std::stringstream const &text)
{
  return from_string<T>(text.str());
}


template<> struct string_traits<std::nullptr_t>
{
  static constexpr bool converts_to_string{false};
  static constexpr bool converts_from_string{false};

  static char *into_buf(char *, char *, std::nullptr_t) = delete;

  [[deprecated("Do not convert nulls.")]] static constexpr zview
  to_buf(char *, char *, std::nullptr_t const &) noexcept
  {
    return {};
  }

  [[deprecated("Do not convert nulls.")]] static constexpr std::size_t
  size_buffer(std::nullptr_t = nullptr) noexcept
  {
    return 0;
  }
  static std::nullptr_t from_string(std::string_view) = delete;
};


template<> struct string_traits<std::nullopt_t>
{
  static constexpr bool converts_to_string{false};
  static constexpr bool converts_from_string{false};

  static char *into_buf(char *, char *, std::nullopt_t) = delete;

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
  static std::nullopt_t from_string(std::string_view) = delete;
};


template<> struct string_traits<std::monostate>
{
  static constexpr bool converts_to_string{false};
  static constexpr bool converts_from_string{false};

  static char *into_buf(char *, char *, std::monostate) = delete;

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
    from_string(std::string_view) = delete;
};


template<> inline constexpr bool is_unquoted_safe<std::nullptr_t>{true};


template<> struct nullness<char const *>
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = false;
  static constexpr bool is_null(char const *t) noexcept
  {
    return t == nullptr;
  }
  static constexpr char const *null() noexcept { return nullptr; }
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
template<> struct string_traits<char const *>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{false};

  static char const *from_string(std::string_view text) = delete;

  static zview to_buf(char *begin, char *end, char const *const &value)
  {
    return generic_to_buf(begin, end, value);
  }

  static char *into_buf(char *begin, char *end, char const *const &value)
  {
    auto const space{end - begin};
    // Count the trailing zero, even though std::strlen() and friends don't.
    auto const len{std::strlen(value) + 1};
    if (space < ptrdiff_t(len))
      throw conversion_overrun{
        "Could not copy string: buffer too small.  " +
        pqxx::internal::state_buffer_overrun(space, len)};
    std::memmove(begin, value, len);
    return begin + len;
  }

  static std::size_t size_buffer(char const *const &value) noexcept
  {
    if (pqxx::is_null(value))
      return 0;
    else
      return std::strlen(value) + 1;
  }
};


template<> struct nullness<char *>
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = false;
  static constexpr bool is_null(char const *t) noexcept
  {
    return t == nullptr;
  }
  static constexpr char const *null() { return nullptr; }
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
template<> struct string_traits<char *>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{false};

  static char *into_buf(char *begin, char *end, char *const &value)
  {
    return string_traits<char const *>::into_buf(begin, end, value);
  }
  static zview to_buf(char *begin, char *end, char *const &value)
  {
    return string_traits<char const *>::to_buf(begin, end, value);
  }
  static std::size_t size_buffer(char *const &value) noexcept
  {
    if (pqxx::is_null(value))
      return 0;
    else
      return string_traits<char const *>::size_buffer(value);
  }

  static char *from_string(std::string_view) = delete;
};


template<std::size_t N> struct nullness<char[N]> : no_null<char[N]>
{};


/// String traits for C-style string constant ("pointer to array of char").
/** @warning This assumes that every array-of-char is a C-style string literal.
 * So, it must include a trailing zero. and it must have static duration.
 */
template<std::size_t N> struct string_traits<char[N]>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{false};

  static constexpr zview
  to_buf(char *, char *, char const (&value)[N]) noexcept
  {
    return zview{value, N - 1};
  }

  static char *into_buf(char *begin, char *end, char const (&value)[N])
  {
    if (std::cmp_less(end - begin, size_buffer(value)))
      throw conversion_overrun{
        "Could not convert char[] to string: too long for buffer."};
    std::memcpy(begin, value, N);
    return begin + N;
  }

  static constexpr std::size_t size_buffer(char const (&)[N]) noexcept
  {
    return N;
  }

  /// Don't allow conversion to this type.
  static void from_string(std::string_view) = delete;
};


template<> struct nullness<std::string> : no_null<std::string>
{};


template<> struct string_traits<std::string>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{true};

  static std::string from_string(std::string_view text)
  {
    return std::string{text};
  }

  static char *into_buf(char *begin, char *end, std::string const &value)
  {
    if (std::cmp_greater_equal(std::size(value), end - begin))
      throw conversion_overrun{
        "Could not convert string to string: too long for buffer."};
    // Include the trailing zero.
    value.copy(begin, std::size(value));
    begin[std::size(value)] = '\0';
    return begin + std::size(value) + 1;
  }

  static zview to_buf(char *begin, char *end, std::string const &value)
  {
    return generic_to_buf(begin, end, value);
  }

  static std::size_t size_buffer(std::string const &value) noexcept
  {
    return std::size(value) + 1;
  }
};


/// There's no real null for `std::string_view`.
/** I'm not sure how clear-cut this is: a `string_view` may have a null
 * data pointer, which is analogous to a null `char` pointer.
 */
template<> struct nullness<std::string_view> : no_null<std::string_view>
{};


/// String traits for `string_view`.
/** @warning This conversion does not store the string's contents anywhere.
 * When you convert a string to a `std::string_view`, _do not access the
 * resulting view after the original string has been destroyed.  The contents
 * will no longer be valid, even though tests may not make this obvious.
 */
template<> struct string_traits<std::string_view>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{true};

  static constexpr std::size_t
  size_buffer(std::string_view const &value) noexcept
  {
    return std::size(value) + 1;
  }

  static char *into_buf(char *begin, char *end, std::string_view const &value)
  {
    if (std::cmp_greater_equal(std::size(value), end - begin))
      throw conversion_overrun{
        "Could not store string_view: too long for buffer."};
    value.copy(begin, std::size(value));
    begin[std::size(value)] = '\0';
    return begin + std::size(value) + 1;
  }

  static zview to_buf(char *begin, char *end, std::string_view const &value)
  {
    // You'd think we could just return the same view but alas, there's no
    // zero-termination on a string_view.
    return generic_to_buf(begin, end, value);
  }

  static std::string_view from_string(std::string_view value) { return value; }
};


template<> struct nullness<zview> : no_null<zview>
{};


/// String traits for `zview`.
template<> struct string_traits<zview>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{false};

  static constexpr std::size_t
  size_buffer(std::string_view const &value) noexcept
  {
    return std::size(value) + 1;
  }

  static char *into_buf(char *begin, char *end, zview const &value)
  {
    auto const size{std::size(value)};
    if (std::cmp_less_equal(end - begin, std::size(value)))
      throw conversion_overrun{"Not enough buffer space to store this zview."};
    value.copy(begin, size);
    begin[size] = '\0';
    return begin + size + 1;
  }

  static std::string_view to_buf(char *begin, char *end, zview const &value)
  {
    char *const stop{into_buf(begin, end, value)};
    return {begin, static_cast<std::size_t>(stop - begin - 1)};
  }

  /// Don't convert to this type.  There may not be a terminating zero.
  /** There is no valid way to figure out here whether there is a terminating
   * zero.  Even if there is one, that may just be the first byte of an
   * entirely separately allocated piece of memory.
   */
  static zview from_string(std::string_view) = delete;
};


template<> struct nullness<std::stringstream> : no_null<std::stringstream>
{};


template<> struct string_traits<std::stringstream>
{
  static constexpr bool converts_to_string{false};
  static constexpr bool converts_from_string{true};

  static std::size_t size_buffer(std::stringstream const &) = delete;

  static std::stringstream from_string(std::string_view text)
  {
    std::stringstream stream;
    stream.write(text.data(), std::streamsize(std::size(text)));
    return stream;
  }

  static char *into_buf(char *, char *, std::stringstream const &) = delete;
  static std::string_view
  to_buf(char *, char *, std::stringstream const &) = delete;
};


template<> struct nullness<std::nullptr_t>
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = true;
  static constexpr bool is_null(std::nullptr_t const &) noexcept
  {
    return true;
  }
  static constexpr std::nullptr_t null() noexcept { return nullptr; }
};


template<> struct nullness<std::nullopt_t>
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = true;
  static constexpr bool is_null(std::nullopt_t const &) noexcept
  {
    return true;
  }
  static constexpr std::nullopt_t null() noexcept { return std::nullopt; }
};


template<> struct nullness<std::monostate>
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = true;
  static constexpr bool is_null(std::monostate const &) noexcept
  {
    return true;
  }
  static constexpr std::monostate null() noexcept { return {}; }
};


template<typename T> struct nullness<std::unique_ptr<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = false;
  static constexpr bool is_null(std::unique_ptr<T> const &t) noexcept
  {
    return not t or pqxx::is_null(*t);
  }
  static constexpr std::unique_ptr<T> null() { return {}; }
};


template<typename T, typename... Args>
struct string_traits<std::unique_ptr<T, Args...>>
{
  static constexpr bool converts_to_string{
    string_traits<T>::converts_to_string};
  static constexpr bool converts_from_string{
    string_traits<T>::converts_from_string};

  static std::unique_ptr<T> from_string(std::string_view text)
  {
    return std::make_unique<T>(string_traits<T>::from_string(text));
  }

  static char *
  into_buf(char *begin, char *end, std::unique_ptr<T, Args...> const &value)
  {
    return string_traits<T>::into_buf(begin, end, *value);
  }

  static zview
  to_buf(char *begin, char *end, std::unique_ptr<T, Args...> const &value)
  {
    if (value)
      return string_traits<T>::to_buf(begin, end, *value);
    else
      return {};
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


template<typename T> struct nullness<std::shared_ptr<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool always_null = false;
  static constexpr bool is_null(std::shared_ptr<T> const &t) noexcept
  {
    return not t or pqxx::is_null(*t);
  }
  static constexpr std::shared_ptr<T> null() { return {}; }
};


template<typename T> struct string_traits<std::shared_ptr<T>>
{
  static constexpr bool converts_to_string{
    string_traits<T>::converts_to_string};
  static constexpr bool converts_from_string{
    string_traits<T>::converts_from_string};

  static std::shared_ptr<T> from_string(std::string_view text)
  {
    return std::make_shared<T>(string_traits<T>::from_string(text));
  }

  static zview to_buf(char *begin, char *end, std::shared_ptr<T> const &value)
  {
    return string_traits<T>::to_buf(begin, end, *value);
  }
  static char *
  into_buf(char *begin, char *end, std::shared_ptr<T> const &value)
  {
    return string_traits<T>::into_buf(begin, end, *value);
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


template<binary DATA> struct nullness<DATA> : no_null<DATA>
{};


template<binary DATA> struct string_traits<DATA>
{
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{true};

  static std::size_t size_buffer(DATA const &value) noexcept
  {
    return internal::size_esc_bin(std::size(value));
  }

  static zview to_buf(char *begin, char *end, DATA const &value)
  {
    return generic_to_buf(begin, end, value);
  }

  static char *into_buf(char *begin, char *end, DATA const &value)
  {
    auto const budget{size_buffer(value)};
    if (std::cmp_less(end - begin, budget))
      throw conversion_overrun{
        "Not enough buffer space to escape binary data."};
    internal::esc_bin(value, begin);
    return begin + budget;
  }

  static DATA from_string(std::string_view text, sl loc = sl::current())
  {
    auto const size{pqxx::internal::size_unesc_bin(std::size(text))};
    bytes buf;
    buf.resize(size);
    // XXX: Use std::as_writable_bytes.
    pqxx::internal::unesc_bin(
      text, reinterpret_cast<std::byte *>(buf.data()), loc);
    return buf;
  }
};
} // namespace pqxx


namespace pqxx::internal
{
// C++20: Use concepts to identify arrays.
/// String traits for SQL arrays.
template<typename Container> struct array_string_traits
{
private:
  using elt_type = std::remove_cvref_t<value_type<Container>>;
  using elt_traits = string_traits<elt_type>;
  static constexpr zview s_null{"NULL"};

public:
  static constexpr bool converts_to_string{true};
  static constexpr bool converts_from_string{false};

  static zview to_buf(char *begin, char *end, Container const &value)
  {
    return generic_to_buf(begin, end, value);
  }

  static char *into_buf(char *begin, char *end, Container const &value)
  {
    assert(begin <= end);
    std::size_t const budget{size_buffer(value)};
    if (std::cmp_less(end - begin, budget))
      throw conversion_overrun{
        "Not enough buffer space to convert array to string."};

    char *here = begin;
    *here++ = '{';

    bool nonempty{false};
    for (auto const &elt : value)
    {
      if (is_null(elt))
      {
        s_null.copy(here, std::size(s_null));
        here += std::size(s_null);
      }
      else if constexpr (is_sql_array<elt_type>)
      {
        // Render nested array in-place.  Then erase the trailing zero.
        here = elt_traits::into_buf(here, end, elt) - 1;
      }
      else if constexpr (is_unquoted_safe<elt_type>)
      {
        // No need to quote or escape.  Just convert the value straight into
        // its place in the array, and "backspace" the trailing zero.
        here = elt_traits::into_buf(here, end, elt) - 1;
      }
      else
      {
        *here++ = '"';

        // Use the tail end of the destination buffer as an intermediate
        // buffer.
        auto const elt_budget{pqxx::size_buffer(elt)};
        assert(elt_budget < static_cast<std::size_t>(end - here));
        for (char const c : elt_traits::to_buf(end - elt_budget, end, elt))
        {
          // We copy the intermediate buffer into the final buffer, char by
          // char, with escaping where necessary.
          // TODO: This will not work for all encodings.  UTF8 & ASCII are OK.
          if (c == '\\' or c == '"')
            *here++ = '\\';
          *here++ = c;
        }
        *here++ = '"';
      }
      *here++ = array_separator<elt_type>;
      nonempty = true;
    }

    // Erase that last comma, if present.
    if (nonempty)
      here--;

    *here++ = '}';
    *here++ = '\0';

    return here;
  }

  static std::size_t size_buffer(Container const &value) noexcept
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
                     // and the one byte for the trailing zero becomes room
                     // for a separator. However, std::size(s_null) doesn't
                     // account for the terminating zero, so add one to make
                     // s_null pay for its own separator.
                     std::size_t const elt_size{
                       pqxx::is_null(elt) ? (std::size(s_null) + 1) :
                                            elt_traits::size_buffer(elt)};
                     return acc + 2 * elt_size + 2;
                   });
  }

  // We don't yet support parsing of array types using from_string.  Doing so
  // would require a reference to the connection.
};
} // namespace pqxx::internal


namespace pqxx
{
template<nonbinary_range T> struct nullness<T> : no_null<T>
{};


template<nonbinary_range T>
struct string_traits<T> : internal::array_string_traits<T>
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


template<typename T> inline std::string to_string(T const &value)
{
  if (is_null(value))
    throw conversion_error{
      "Attempt to convert null " + std::string{type_name<T>} +
      " to a string."};

  if constexpr (nullness<std::remove_cvref_t<T>>::always_null)
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
    buf.resize(size_buffer(value));
    auto const data{buf.data()};
    auto const end{
      string_traits<T>::into_buf(data, data + std::size(buf), value)};
    buf.resize(static_cast<std::size_t>(end - data - 1));
    return buf;
  }
}


template<> inline std::string to_string(float const &value)
{
  return pqxx::internal::to_string_float(value);
}
template<> inline std::string to_string(double const &value)
{
  return pqxx::internal::to_string_float(value);
}
template<> inline std::string to_string(long double const &value)
{
  return pqxx::internal::to_string_float(value);
}
template<> inline std::string to_string(std::stringstream const &value)
{
  return value.str();
}


template<typename T> inline void into_string(T const &value, std::string &out)
{
  if (is_null(value))
    throw conversion_error{
      "Attempt to convert null " + type_name<T> + " to a string."};

  // We can't just reserve() data; modifying the terminating zero leads to
  // undefined behaviour.
  out.resize(size_buffer(value) + 1);
  auto const data{out.data()};
  auto const end{
    string_traits<T>::into_buf(data, data + std::size(out), value)};
  out.resize(static_cast<std::size_t>(end - data - 1));
}
} // namespace pqxx
