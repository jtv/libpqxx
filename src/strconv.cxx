/** Implementation of string conversions.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
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
#include <functional>
#include <limits>
#include <locale>
#include <string_view>
#include <system_error>

#if __has_include(<cxxabi.h>)
#  include <cxxabi.h>
#endif

#include "pqxx/except"
#include "pqxx/strconv"


namespace
{
/// Do we have fully functional thread_local support?
/** When building with libcxxrt on clang, you can't create thread_local objects
 * of non-POD types.  Any attempt will result in a link error.
 */
constexpr bool have_thread_local
{
#if defined(PQXX_HAVE_THREAD_LOCAL)
  true
#else
  false
#endif
};

/// String comparison between string_view.
constexpr inline bool equal(std::string_view lhs, std::string_view rhs)
{
  return lhs.compare(rhs) == 0;
}


/// The lowest possible value of integral type T.
template<typename T> constexpr T bottom{std::numeric_limits<T>::min()};

/// The highest possible value of integral type T.
template<typename T> constexpr T top{std::numeric_limits<T>::max()};

/// Write nonnegative integral value at end of buffer.  Return start.
/** Assumes a sufficiently large buffer.
 *
 * Includes a single trailing null byte, right before @c *end.
 */
template<typename T> constexpr inline char *nonneg_to_buf(char *end, T value)
{
  char *pos = end;
  *--pos = '\0';
  do
  {
    *--pos = pqxx::internal::number_to_digit(int(value % 10));
    value = T(value / 10);
  } while (value > 0);
  return pos;
}


/// Write negative version of value at end of buffer.  Return start.
/** Like @c nonneg_to_buf, but prefixes a minus sign.
 */
template<typename T> constexpr inline char *neg_to_buf(char *end, T value)
{
  char *pos = nonneg_to_buf(end, value);
  *--pos = '-';
  return pos;
}


/// Write lowest possible negative value at end of buffer.
/** Like @c neg_to_buf, but for the special case of the bottom value.
 */
template<typename T> constexpr inline char *bottom_to_buf(char *end)
{
  static_assert(std::is_signed_v<T>);

  // This is the hard case.  In two's-complement systems, which includes
  // any modern-day system I can think of, a signed type's bottom value
  // has no positive equivalent.  Luckily the C++ standards committee can't
  // think of any exceptions either, so it's the required representation as
  // of C++20.  We'll assume it right now, while still on C++17.
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


#if defined(PQXX_HAVE_CHARCONV_INT) || defined(PQXX_HAVE_CHARCONV_FLOAT)
/// Call to_chars, report errors as exceptions, add zero, return pointer.
template<typename T>
[[maybe_unused]] inline char *
wrap_to_chars(char *begin, char *end, T const &value)
{
  auto res{std::to_chars(begin, end - 1, value)};
  if (res.ec != std::errc())
    switch (res.ec)
    {
    case std::errc::value_too_large:
      throw pqxx::conversion_overrun{
        "Could not convert " + pqxx::type_name<T> +
        " to string: "
        "buffer too small (" +
        pqxx::to_string(end - begin) + " bytes)."};
    default:
      throw pqxx::conversion_error{
        "Could not convert " + pqxx::type_name<T> + " to string."};
    }
  // No need to check for overrun here: we never even told to_chars about that
  // last byte in the buffer, so it didn't get used up.
  *res.ptr++ = '\0';
  return res.ptr;
}
#endif
} // namespace


namespace pqxx::internal
{
template<typename T>
zview integral_traits<T>::to_buf(char *begin, char *end, T const &value)
{
  static_assert(std::is_integral_v<T>);
  auto const space{end - begin},
    need{static_cast<ptrdiff_t>(size_buffer(value))};
  if (space < need)
    throw conversion_overrun{
      "Could not convert " + type_name<T> +
      " to string: "
      "buffer too small.  " +
      pqxx::internal::state_buffer_overrun(space, need)};

  char *pos;
  if constexpr (std::is_unsigned_v<T>)
    pos = nonneg_to_buf(end, value);
  else if (value >= 0)
    pos = nonneg_to_buf(end, value);
  else if (value > bottom<T>)
    pos = neg_to_buf(end, -value);
  else
    pos = bottom_to_buf<T>(end);

  return zview{pos, end - pos - 1};
}


template zview integral_traits<short>::to_buf(char *, char *, short const &);
template zview integral_traits<unsigned short>::to_buf(
  char *, char *, unsigned short const &);
template zview integral_traits<int>::to_buf(char *, char *, int const &);
template zview
integral_traits<unsigned>::to_buf(char *, char *, unsigned const &);
template zview integral_traits<long>::to_buf(char *, char *, long const &);
template zview
integral_traits<unsigned long>::to_buf(char *, char *, unsigned long const &);
template zview
integral_traits<long long>::to_buf(char *, char *, long long const &);
template zview integral_traits<unsigned long long>::to_buf(
  char *, char *, unsigned long long const &);


template<typename T>
char *integral_traits<T>::into_buf(char *begin, char *end, T const &value)
{
#if defined(PQXX_HAVE_CHARCONV_INT)
  // This is exactly what to_chars is good at.  Trust standard library
  // implementers to optimise better than we can.
  return wrap_to_chars(begin, end, value);
#else
  return generic_into_buf(begin, end, value);
#endif
}


template char *integral_traits<short>::into_buf(char *, char *, short const &);
template char *integral_traits<unsigned short>::into_buf(
  char *, char *, unsigned short const &);
template char *integral_traits<int>::into_buf(char *, char *, int const &);
template char *
integral_traits<unsigned>::into_buf(char *, char *, unsigned const &);
template char *integral_traits<long>::into_buf(char *, char *, long const &);
template char *integral_traits<unsigned long>::into_buf(
  char *, char *, unsigned long const &);
template char *
integral_traits<long long>::into_buf(char *, char *, long long const &);
template char *integral_traits<unsigned long long>::into_buf(
  char *, char *, unsigned long long const &);
} // namespace pqxx::internal


namespace pqxx::internal
{
std::string demangle_type_name(char const raw[])
{
#if defined(PQXX_HAVE_CXA_DEMANGLE)
  int status{0};
  char *const name_ptr{abi::__cxa_demangle(raw, nullptr, nullptr, &status)};
  if (status != 0)
  {
    // Not great C++ style, but turned out a lot easier than std::unique_ptr.
    // The std::free() can throw an exception, even if C's free() cannot.
    // This led to noexcept warnings so complex that gcc itself, prior to
    // version 10, got a little confused.
    std::free(name_ptr);
    throw std::runtime_error(
      std::string{"Could not demangle type name '"} + raw +
      "': __cxa_demangle failed.");
  }
  // If this fails, we'll leak `name_ptr`.  Yes, we could handle that more
  // robustly.  But are there any circumstances where it would really matter?
  std::string const name{name_ptr};
  std::free(name_ptr);
  return name;
#else
  return std::string{raw};
#endif
}

void throw_null_conversion(std::string const &type)
{
  throw conversion_error{"Attempt to convert null to " + type + "."};
}


std::string state_buffer_overrun(int have_bytes, int need_bytes)
{
  // We convert these in standard library terms, not for the localisation
  // so much as to avoid "error cycles," if these values in turn should fail
  // to get enough buffer space.
  std::stringstream have, need;
  have << have_bytes;
  need << need_bytes;
  return "Have " + have.str() + " bytes, need " + need.str() + ".";
}
} // namespace pqxx::internal


namespace
{
#if defined(PQXX_HAVE_CHARCONV_INT) || defined(PQXX_HAVE_CHARCONV_FLOAT)
template<typename TYPE>
[[maybe_unused]] inline TYPE from_string_arithmetic(std::string_view in)
{
  char const *begin;

  // Skip whitespace.  This is not the proper way to do it, but I see no way
  // that any of the supported encodings could ever produce a valid character
  // whose byte sequence would confuse this code.
  for (begin = in.data();
       begin < in.end() and (*begin == ' ' or *begin == '\t'); ++begin)
    ;

  auto const end{in.data() + in.size()};
  TYPE out;
  auto const res{std::from_chars(begin, end, out)};
  if (res.ec == std::errc() and res.ptr == end)
    return out;

  std::string msg;
  if (res.ec == std::errc())
  {
    msg = "Could not parse full string.";
  }
  else
    switch (res.ec)
    {
    case std::errc::result_out_of_range: msg = "Value out of range."; break;
    case std::errc::invalid_argument: msg = "Invalid argument."; break;
    default: break;
    }

  auto const base{
    "Could not convert '" + std::string(in) +
    "' "
    "to " +
    pqxx::type_name<TYPE>};
  if (msg.empty())
    throw pqxx::conversion_error{base + "."};
  else
    throw pqxx::conversion_error{base + ": " + msg};
}
#endif
} // namespace


namespace
{
#if !defined(PQXX_HAVE_CHARCONV_INT)
[[noreturn, maybe_unused]] void report_overflow()
{
  throw pqxx::conversion_error{
    "Could not convert string to integer: value out of range."};
}


/// Return 10*n, or throw exception if it overflows.
template<typename T>
[[maybe_unused]] constexpr inline T safe_multiply_by_ten(T n)
{
  using limits = std::numeric_limits<T>;

  constexpr T ten{10};
  constexpr T high_threshold(std::numeric_limits<T>::max() / ten);
  if (n > high_threshold)
    report_overflow();
  if constexpr (limits::is_signed)
  {
    constexpr T low_threshold(std::numeric_limits<T>::min() / ten);
    if (low_threshold > n)
      report_overflow();
  }
  return T(n * ten);
}


/// Add digit d to nonnegative n, or throw exception if it overflows.
template<typename T>
[[maybe_unused]] constexpr inline T safe_add_digit(T n, T d)
{
  T const high_threshold{static_cast<T>(std::numeric_limits<T>::max() - d)};
  if (n > high_threshold)
    report_overflow();
  return static_cast<T>(n + d);
}


/// Subtract digit d to nonpositive n, or throw exception if it overflows.
template<typename T>
[[maybe_unused]] constexpr inline T safe_sub_digit(T n, T d)
{
  T const low_threshold{static_cast<T>(std::numeric_limits<T>::min() + d)};
  if (n < low_threshold)
    report_overflow();
  return static_cast<T>(n - d);
}


/// For use in string parsing: add new numeric digit to intermediate value.
template<typename L, typename R>
[[maybe_unused]] constexpr inline L absorb_digit_positive(L value, R digit)
{
  return safe_add_digit(safe_multiply_by_ten(value), L(digit));
}


/// For use in string parsing: subtract digit from intermediate value.
template<typename L, typename R>
[[maybe_unused]] constexpr inline L absorb_digit_negative(L value, R digit)
{
  return safe_sub_digit(safe_multiply_by_ten(value), L(digit));
}


/// Compute numeric value of given textual digit (assuming that it is a digit).
[[maybe_unused]] constexpr int digit_to_number(char c) noexcept
{
  return c - '0';
}


template<typename T>
[[maybe_unused]] constexpr T from_string_integer(std::string_view text)
{
  if (text.size() == 0)
    throw pqxx::conversion_error{
      "Attempt to convert empty string to " + pqxx::type_name<T> + "."};

  char const *const data{text.data()};
  std::size_t i{0};

  // Skip whitespace.  This is not the proper way to do it, but I see no way
  // that any of the supported encodings could ever produce a valid character
  // whose byte sequence would confuse this code.
  //
  // Why skip whitespace?  Because that's how integral conversions are meant to
  // work _for composite types._  I see no clean way to support leading
  // whitespace there without putting the code in here.  A shame about the
  // overhead, modest as it is, for the normal case.
  for (; i < text.size() and (data[i] == ' ' or data[i] == '\t'); ++i)
    ;
  if (i == text.size())
    throw pqxx::conversion_error{
      "Converting string to " + pqxx::type_name<T> +
      ", but it contains only whitespace."};

  char const initial{data[i]};
  T result{0};

  if (isdigit(initial))
  {
    for (; isdigit(data[i]); ++i)
      result = absorb_digit_positive(result, digit_to_number(data[i]));
  }
  else if (initial == '-')
  {
    if constexpr (not std::is_signed_v<T>)
      throw pqxx::conversion_error{
        "Attempt to convert negative value to " + pqxx::type_name<T> + "."};

    ++i;
    if (i >= text.size())
      throw pqxx::conversion_error{
        "Converting string to " + pqxx::type_name<T> +
        ", but it contains only a sign."};
    for (; i < text.size() and isdigit(data[i]); ++i)
      result = absorb_digit_negative(result, digit_to_number(data[i]));
  }
  else
  {
    throw pqxx::conversion_error{
      "Could not convert string to " + pqxx::type_name<T> +
      ": "
      "'" +
      std::string{text} + "'."};
  }

  if (i < text.size())
    throw pqxx::conversion_error{
      "Unexpected text after " + pqxx::type_name<T> +
      ": "
      "'" +
      std::string{text} + "'."};

  return result;
}
#endif // !PQXX_HAVE_CHARCONV_INT
} // namespace


namespace
{
[[maybe_unused]] constexpr bool
valid_infinity_string(std::string_view text) noexcept
{
  return equal("infinity", text) or equal("Infinity", text) or
         equal("INFINITY", text) or equal("inf", text);
  equal("Inf", text);
  equal("INF", text);
}
} // namespace


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace
{
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
template<typename T> class dumb_stringstream : public std::stringstream
{
public:
  // Do not initialise the base-class object using "stringstream{}" (with curly
  // braces): that breaks on Visual C++.  The classic "stringstream()" syntax
  // (with parentheses) does work.
  dumb_stringstream()
  {
    this->imbue(std::locale::classic());
    this->precision(std::numeric_limits<T>::max_digits10);
  }
};


template<typename F>
inline bool from_dumb_stringstream(
  dumb_stringstream<F> &s, F &result, std::string_view text)
{
  s.str(std::string{text});
  return static_cast<bool>(s >> result);
}


// These are hard, and popular compilers do not yet implement std::from_chars.
template<typename T> inline T from_string_awful_float(std::string_view text)
{
  if (text.empty())
    throw pqxx::conversion_error{
      "Trying to convert empty string to " + pqxx::type_name<T> + "."};

  bool ok{false};
  T result;

  switch (text[0])
  {
  case 'N':
  case 'n':
    // Accept "NaN," "nan," etc.
    ok =
      (text.size() == 3 and (text[1] == 'A' or text[1] == 'a') and
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
    else
    {
      if (have_thread_local)
      {
        thread_local dumb_stringstream<T> S;
        // Visual Studio 2017 seems to fail on repeated conversions if the
        // clear() is done before the seekg().  Still don't know why!  See #124
        // and #125.
        S.seekg(0);
        S.clear();
        ok = from_dumb_stringstream(S, result, text);
      }
      else
      {
        dumb_stringstream<T> S;
        ok = from_dumb_stringstream(S, result, text);
      }
    }
    break;
  }

  if (not ok)
    throw pqxx::conversion_error{
      "Could not convert string to numeric value: '" + std::string{text} +
      "'."};

  return result;
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_FLOAT


namespace pqxx::internal
{
/// Floating-point to_buf implemented in terms of to_string.
template<typename T>
zview float_traits<T>::to_buf(char *begin, char *end, T const &value)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  {
    // Definitely prefer to let the standard library handle this!
    auto const ptr{wrap_to_chars(begin, end, value)};
    return zview{begin, std::size_t(ptr - begin - 1)};
  }
#else
  {
    // Implement it ourselves.  Weird detail: since this workaround is based on
    // std::stringstream, which produces a std::string, it's actually easier to
    // build the to_buf() on top of the to_string() than the other way around.
    if (std::isnan(value))
      return zview{"nan"};
    if (std::isinf(value))
      return zview{(value > 0) ? "infinity" : "-infinity"};
    auto text{to_string_float(value)};
    auto have{end - begin};
    auto need{text.size() + 1};
    if (need > std::size_t(have))
      throw conversion_error{
        "Could not convert floating-point number to string: "
        "buffer too small.  " +
        state_buffer_overrun(have, need)};
    text.copy(begin, need);
    return zview{begin, text.size()};
  }
#endif
}


template zview float_traits<float>::to_buf(char *, char *, float const &);
template zview float_traits<double>::to_buf(char *, char *, double const &);
template zview
float_traits<long double>::to_buf(char *, char *, long double const &);


template<typename T>
char *float_traits<T>::into_buf(char *begin, char *end, T const &value)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  return wrap_to_chars(begin, end, value);
#else
  return generic_into_buf(begin, end, value);
#endif
}


template char *float_traits<float>::into_buf(char *, char *, float const &);
template char *float_traits<double>::into_buf(char *, char *, double const &);
template char *
float_traits<long double>::into_buf(char *, char *, long double const &);


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
template<typename F>
inline std::string to_dumb_stringstream(dumb_stringstream<F> &s, F value)
{
  s.str("");
  s << value;
  return s.str();
}
#endif


/// Floating-point implementations for @c pqxx::to_string().
template<typename T> std::string to_string_float(T value)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  {
    constexpr auto space{float_traits<T>::size_buffer(value)};
    std::string buf;
    buf.resize(space);
    std::string_view const view{
      float_traits<T>::to_buf(buf.data(), buf.data() + space, value)};
    buf.resize(view.end() - view.begin());
    return buf;
  }
#else
  {
    // In this rare case, we can convert to std::string but not to a simple
    // buffer.  So, implement to_buf in terms of to_string instead of the other
    // way around.
    if (have_thread_local)
    {
      thread_local dumb_stringstream<T> s;
      return to_dumb_stringstream(s, value);
    }
    else
    {
      dumb_stringstream<T> s;
      return to_dumb_stringstream(s, value);
    }
  }
#endif
}
} // namespace pqxx::internal


namespace pqxx::internal
{
template<typename T> T integral_traits<T>::from_string(std::string_view text)
{
#if defined(PQXX_HAVE_CHARCONV_INT)
  return from_string_arithmetic<T>(text);
#else
  return from_string_integer<T>(text);
#endif
}

template short integral_traits<short>::from_string(std::string_view);
template unsigned short
  integral_traits<unsigned short>::from_string(std::string_view);
template int integral_traits<int>::from_string(std::string_view);
template unsigned integral_traits<unsigned>::from_string(std::string_view);
template long integral_traits<long>::from_string(std::string_view);
template unsigned long
  integral_traits<unsigned long>::from_string(std::string_view);
template long long integral_traits<long long>::from_string(std::string_view);
template unsigned long long
  integral_traits<unsigned long long>::from_string(std::string_view);


template<typename T> T float_traits<T>::from_string(std::string_view text)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  return from_string_arithmetic<T>(text);
#else
  return from_string_awful_float<T>(text);
#endif
}


template float float_traits<float>::from_string(std::string_view);
template double float_traits<double>::from_string(std::string_view);
template long double float_traits<long double>::from_string(std::string_view);


template std::string to_string_float(float);
template std::string to_string_float(double);
template std::string to_string_float(long double);
} // namespace pqxx::internal


bool pqxx::string_traits<bool>::from_string(std::string_view text)
{
  bool OK, result;

  switch (text.size())
  {
  case 0:
    result = false;
    OK = true;
    break;

  case 1:
    switch (text[0])
    {
    case 'f':
    case 'F':
    case '0':
      result = false;
      OK = true;
      break;

    case 't':
    case 'T':
    case '1':
      result = true;
      OK = true;
      break;

    default: OK = false; break;
    }
    break;

  case 4:
    result = true;
    OK = (equal(text, "true") or equal(text, "TRUE"));
    break;

  case 5:
    result = false;
    OK = (equal(text, "false") or equal(text, "FALSE"));
    break;

  default: OK = false; break;
  }

  if (not OK)
    throw conversion_error{
      "Failed conversion to bool: '" + std::string{text} + "'."};

  return result;
}
