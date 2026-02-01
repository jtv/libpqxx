/** Implementation of string conversions.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <format>
#include <functional>
#include <limits>
#include <locale>
#include <string_view>
#include <system_error>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/except.hxx"
#include "pqxx/strconv.hxx"

#include "pqxx/internal/header-post.hxx"


using namespace std::literals;

namespace
{
/// The lowest possible value of integral type T.
template<pqxx::internal::integer T>
constexpr T bottom{std::numeric_limits<T>::min()};

/// The highest possible value of integral type T.
template<pqxx::internal::integer T>
constexpr T top{std::numeric_limits<T>::max()};

/// Write nonnegative integral value at end of buffer.  Return start.
/** Assumes a sufficiently large buffer.
 *
 * Includes a single trailing null byte, right before @c *end.
 */
template<pqxx::internal::integer T>
constexpr inline char *nonneg_to_buf(char *end, T value)
{
  assert(std::cmp_greater_equal(value, 0));
  constexpr int ten{10};
  // Seeming bug in clang-tidy rule: it thinks we can make pos a "char const *"
  // instead of a plain "char *".  I don't see how.
  // NOLINTNEXTLINE(misc-const-correctness)
  char *pos = end;
  do {
    *--pos = pqxx::internal::number_to_digit(int(value % ten));
    value = T(value / ten);
  } while (value > 0);
  return pos;
}


/// Write negative version of value at end of buffer.  Return start.
/** Like @c nonneg_to_buf, but prefixes a minus sign.
 */
template<pqxx::internal::integer T>
constexpr inline char *neg_to_buf(char *end, T value)
{
  assert(std::cmp_greater_equal(value, 0));
  // Seeming bug in clang-tidy rule: it thinks we can make pos a "char const *"
  // instead of a plain "char *".  I don't see how.
  // NOLINTNEXTLINE(misc-const-correctness)
  char *pos{nonneg_to_buf(end, value)};
  *--pos = '-';
  return pos;
}


/// Write lowest possible negative value at end of buffer.
/** Like @c neg_to_buf, but for the special case of the bottom value.
 */
template<pqxx::internal::integer T>
constexpr inline char *bottom_to_buf(char *end)
{
  static_assert(std::is_signed_v<T>);

  // This is the hard case.  In two's-complement systems, which includes
  // any modern-day system I can think of, a signed type's bottom value
  // has no positive equivalent.  Luckily the C++ standards committee can't
  // think of any exceptions either, so it's the required representation as
  // of C++20.
  static_assert(-(bottom<T> + 1) == top<T>);

  // The unsigned version of T does have the unsigned version of bottom.
  using unsigned_t = std::make_unsigned_t<T>;

  // Careful though.  If we tried to negate value in order to promote to
  // unsigned_t, the value will overflow, which means behaviour is
  // undefined.  Promotion of a negative value to an unsigned type is
  // well-defined, given a representation, so let's do that:
  constexpr auto positive{static_cast<unsigned_t>(bottom<T>)};

  // As luck would have it, in two's complement, this gives us exactly the
  // value we want.
  static_assert(positive == top<unsigned_t> / 2 + 1);

  // So the only thing we need to do differently from the regular negative
  // case is to skip that overflowing negation and promote to an unsigned type!
  return neg_to_buf(end, positive);
}


template<typename T>
concept arith = pqxx::internal::integer<T> or std::floating_point<T>;


/// Call to_chars, report errors as exceptions, add zero, return pointer.
template<arith T>
inline char *wrap_to_chars(std::span<char> buf, T const &value, pqxx::sl loc)
{
  auto const begin{std::data(buf)}, end{begin + std::size(buf)};
  auto res{std::to_chars(begin, end, value)};
  if (res.ec == std::errc()) [[likely]]
    return res.ptr;
  else if (res.ec == std::errc::value_too_large)
    throw pqxx::conversion_overrun{
      std::format(
        "Could not convert {} to string: buffer too small ({} bytes).",
        pqxx::name_type<T>(), std::size(buf)),
      loc};
  else
    throw pqxx::conversion_error{
      std::format("Could not convert {} to string.", pqxx::name_type<T>()),
      loc};
}
} // namespace

namespace pqxx::internal
{
// TODO: Equivalents for converting a null in the other direction.
PQXX_COLD void throw_null_conversion(std::string const &type, sl loc)
{
  throw conversion_error{
    std::format("Attempted to convert SQL null to {}.", type), loc};
}


PQXX_COLD void throw_null_conversion(std::string_view type, sl loc)
{
  throw conversion_error{
    std::format("Attempted to convert SQL null to {}.", type), loc};
}


PQXX_COLD std::string state_buffer_overrun(int have_bytes, int need_bytes)
{
  // We convert these in standard library terms, to avoid "error cycles," if
  // these values in turn should fail to get enough buffer space.
  return std::format("Have {} bytes, need {}.", have_bytes, need_bytes);
}
} // namespace pqxx::internal


namespace
{
template<arith TYPE>
inline TYPE from_string_arithmetic(std::string_view in, pqxx::ctx c)
{
  char const *here{std::data(in)};
  auto const end{std::data(in) + std::size(in)};

  // Skip whitespace.  This is not the proper way to do it, but I see no way
  // that any of the supported encodings could ever produce a valid character
  // whose byte sequence would confuse this code.
  while (here < end and (*here == ' ' or *here == '\t')) ++here;

  TYPE out{};
  auto const res{std::from_chars(here, end, out)};
  if (res.ec == std::errc() and res.ptr == end) [[likely]]
    return out;

  std::string msg;
  if (res.ec == std::errc())
  {
    msg = "Could not parse full string.";
  }
  else
  {
    switch (res.ec)
    {
    case std::errc::result_out_of_range: msg = "Value out of range."; break;
    case std::errc::invalid_argument: msg = "Invalid argument."; break;
    default: break;
    }
  }

  auto const base{std::format(
    "Could not convert '{}' to {}", std::string(in), pqxx::name_type<TYPE>())};
  if (std::empty(msg))
    throw pqxx::conversion_error{std::format("{}.", base), c.loc};
  else
    throw pqxx::conversion_error{std::format("{}: {}", base, msg), c.loc};
}


// TODO: Remove this workaround once compilers have float charconv.
#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
constexpr bool valid_infinity_string(std::string_view text) noexcept
{
  return text == "inf" or text == "infinity" or text == "INFINITY" or
         text == "Infinity";
}


/// Wrapper for std::stringstream with C locale.
/** We use this to work around missing std::to_chars for floating-point types.
 *
 * Initialising the stream (including locale and tweaked precision) seems to
 * be expensive.  So, create thread-local instances which we re-use.  It's a
 * lockless way of keeping global variables thread-safe, basically.
 *
 * The stream initialisation happens once per thread, in the constructor.
 * And that's why we need to wrap this in a class.  We can't just do it at the
 * call site, or we'd still be doing it for every call.
 */
template<arith T> class dumb_stringstream : public std::stringstream
{
public:
  // Do not initialise the base-class object using "stringstream{}" (with curly
  // braces): that breaks on Visual C++.  The classic "stringstream()" syntax
  // (with parentheses) does work.
  PQXX_COLD dumb_stringstream()
  {
    this->imbue(std::locale::classic());
    this->precision(std::numeric_limits<T>::max_digits10);
  }
};


template<arith F>
PQXX_COLD inline bool from_dumb_stringstream(
  dumb_stringstream<F> &s, F &result, std::string_view text)
{
  s.str(std::string{text});
  return static_cast<bool>(s >> result);
}


// These are hard, and some popular compilers still lack std::from_chars.
template<std::floating_point T>
PQXX_COLD inline T from_string_awful_float(std::string_view text, pqxx::ctx c)
{
  if (std::empty(text))
    throw pqxx::conversion_error{
      std::format(
        "Trying to convert empty string to {}.", pqxx::name_type<T>()),
      c.loc};

  bool ok{false};
  T result;

  switch (text[0])
  {
  case 'N':
  case 'n':
    // Accept "NaN," "nan," etc.
    ok =
      (std::size(text) == 3 and (text[1] == 'A' or text[1] == 'a') and
       (text[2] == 'N' or text[2] == 'n'));
    result = std::numeric_limits<T>::quiet_NaN();
    break;

  case 'I':
  case 'i':
    ok = valid_infinity_string(text);
    result = std::numeric_limits<T>::infinity();
    break;

  default:
    if (text[0] == '-' and valid_infinity_string(text.substr(1)))
    {
      ok = true;
      result = -std::numeric_limits<T>::infinity();
    }
    else [[likely]]
    {
      thread_local dumb_stringstream<T> S;
      // Visual Studio 2017 seems to fail on repeated conversions if the
      // clear() is done before the seekg().  Still don't know why!  See #124
      // and #125.
      S.seekg(0);
      S.clear();
      ok = from_dumb_stringstream(S, result, text);
    }
    break;
  }

  if (not ok)
    throw pqxx::conversion_error{
      std::format(
        "Could not convert string to numeric value: '{}'.", std::string(text)),
      c.loc};

  return result;
}
#endif // !PQXX_HAVE_CHARCONV_FLOAT


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
template<arith F>
PQXX_COLD inline std::string
to_dumb_stringstream(dumb_stringstream<F> &s, F value)
{
  s.str("");
  s << value;
  return s.str();
}
#endif
} // namespace


namespace pqxx::internal
{
/// Floating-point implementations for @c pqxx::to_string().
template<std::floating_point T>
std::string to_string_float(T value, [[maybe_unused]] ctx c)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  {
    static constexpr auto space{pqxx::size_buffer(value)};
    std::string buf;
    buf.resize(space);
    std::string_view const view{to_buf(buf, value, c)};
    buf.resize(static_cast<std::size_t>(std::size(view)));
    return buf;
  }
#else
  return std::format("{}", value);
#endif
}


template<std::floating_point T>
T float_string_traits<T>::from_string(std::string_view text, pqxx::ctx c)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  return from_string_arithmetic<T>(text, c);
#else
  return from_string_awful_float<T>(text, c);
#endif
}


template<std::floating_point T>
std::string_view
float_string_traits<T>::to_buf(std::span<char> buf, T const &value, ctx c)
{
  auto const begin{std::data(buf)};
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  {
    // Definitely prefer to let the standard library handle this!
    auto const ptr{wrap_to_chars(buf, value, c.loc)};
    return {begin, std::size_t(ptr - begin)};
  }
#else
  {
    auto const end{begin + std::size(buf)};
    auto const res{std::format_to_n(begin, end - begin, "{}", value)};
    if (std::cmp_greater_equal(res.size, end - begin))
      throw conversion_overrun{
        std::format(
          "Buffer too small for converting {} to string.", name_type<T>()),
        c.loc};
    return {begin, static_cast<std::size_t>(res.size)};
  }
#endif
}


template struct float_string_traits<float>;
template struct float_string_traits<double>;
template struct float_string_traits<long double>;
} // namespace pqxx::internal


namespace pqxx::internal
{
template<pqxx::internal::integer TYPE>
TYPE integer_string_traits<TYPE>::from_string(std::string_view text, ctx c)
{
  return from_string_arithmetic<TYPE>(text, c);
}

template<pqxx::internal::integer TYPE>
inline std::string_view
// NOLINTNEXTLINE(readability-non-const-parameter)
integer_string_traits<TYPE>::to_buf(
  std::span<char> buf, TYPE const &value, ctx c)
{
  static_assert(std::is_integral_v<TYPE>);
  auto const space{std::size(buf)}, need{size_buffer(value)};
  if (std::cmp_less(space, need))
    throw conversion_overrun{
      std::format(
        "Could not convert {} to string: buffer too small.  {}",
        name_type<TYPE>(), pqxx::internal::state_buffer_overrun(space, need)),
      c.loc};

  auto const end{std::data(buf) + std::size(buf)};
  char const *const pos{[end, &value]() {
    if constexpr (std::is_unsigned_v<TYPE>)
      return nonneg_to_buf(end, value);
    else if (value >= 0)
      return nonneg_to_buf(end, value);
    else if (value > bottom<TYPE>)
      return neg_to_buf(end, -value);
    else
      return bottom_to_buf<TYPE>(end);
  }()};
  return {pos, static_cast<std::size_t>(end - pos)};
}

template struct integer_string_traits<short>;
template struct integer_string_traits<unsigned short>;
template struct integer_string_traits<int>;
template struct integer_string_traits<unsigned>;
template struct integer_string_traits<long>;
template struct integer_string_traits<unsigned long>;
template struct integer_string_traits<long long>;
template struct integer_string_traits<unsigned long long>;
} // namespace pqxx::internal


namespace pqxx::internal
{
template std::string to_string_float(float, ctx);
template std::string to_string_float(double, ctx);
template std::string to_string_float(long double, ctx);
} // namespace pqxx::internal


bool pqxx::string_traits<bool>::from_string(std::string_view text, ctx c)
{
  std::optional<bool> result;

  // TODO: Don't really need to handle all these formats.
  switch (std::size(text))
  {
  case 0: result = false; break;

  case 1:
    switch (text[0])
    {
    case 'f':
    case 'F':
    case '0': result = false; break;

    case 't':
    case 'T':
    case '1': result = true; break;

    default: break;
    }
    break;

  case std::size("true"sv):
    if (text == "true" or text == "TRUE")
      result = true;
    break;

  case std::size("false"sv):
    if (text == "false" or text == "FALSE")
      result = false;
    break;

  default: break;
  }

  if (result)
    return *result;
  else
    throw conversion_error{std::format(
      "Failed conversion to bool ({}): '{}'.", pqxx::source_loc(c.loc),
      std::string{text})};
}
