/** Implementation of string conversions.
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen.
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

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/except.hxx"
#include "pqxx/internal/concat.hxx"
#include "pqxx/strconv.hxx"

#include "pqxx/internal/header-post.hxx"


using namespace std::literals;

namespace
{
#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
/// Do we have fully functional thread_local support?
/** When building with libcxxrt on clang, you can't create thread_local objects
 * of non-POD types.  Any attempt will result in a link error.
 */
constexpr bool have_thread_local{
#  if defined(PQXX_HAVE_THREAD_LOCAL)
  true
#  else
  false
#  endif
};
#endif


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
  constexpr int ten{10};
  char *pos = end;
  *--pos = '\0';
  do {
    *--pos = pqxx::internal::number_to_digit(int(value % ten));
    value = T(value / ten);
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


/// Call to_chars, report errors as exceptions, add zero, return pointer.
template<typename T>
inline char *wrap_to_chars(char *begin, char *end, T const &value)
{
  auto res{std::to_chars(begin, end - 1, value)};
  if (res.ec != std::errc()) [[unlikely]]
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
} // namespace


namespace pqxx
{
template<pqxx::internal::integer T>
// NOLINTNEXTLINE(readability-non-const-parameter)
zview string_traits<T>::to_buf(char *begin, char *end, T const &value)
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

  char *const pos{[end, &value]() {
    if constexpr (std::is_unsigned_v<T>)
      return nonneg_to_buf(end, value);
    else if (value >= 0)
      return nonneg_to_buf(end, value);
    else if (value > bottom<T>)
      return neg_to_buf(end, -value);
    else
      return bottom_to_buf<T>(end);
  }()};
  return {pos, end - pos - 1};
}


template zview string_traits<short>::to_buf(char *, char *, short const &);
template zview string_traits<unsigned short>::to_buf(
  char *, char *, unsigned short const &);
template zview string_traits<int>::to_buf(char *, char *, int const &);
template zview
string_traits<unsigned>::to_buf(char *, char *, unsigned const &);
template zview string_traits<long>::to_buf(char *, char *, long const &);
template zview
string_traits<unsigned long>::to_buf(char *, char *, unsigned long const &);
template zview
string_traits<long long>::to_buf(char *, char *, long long const &);
template zview string_traits<unsigned long long>::to_buf(
  char *, char *, unsigned long long const &);


template<pqxx::internal::integer T>
char *string_traits<T>::into_buf(char *begin, char *end, T const &value)
{
  // This is exactly what to_chars is good at.  Trust standard library
  // implementers to optimise better than we can.
  return wrap_to_chars(begin, end, value);
}


template char *string_traits<short>::into_buf(char *, char *, short const &);
template char *string_traits<unsigned short>::into_buf(
  char *, char *, unsigned short const &);
template char *string_traits<int>::into_buf(char *, char *, int const &);
template char *
string_traits<unsigned>::into_buf(char *, char *, unsigned const &);
template char *string_traits<long>::into_buf(char *, char *, long const &);
template char *string_traits<unsigned long>::into_buf(
  char *, char *, unsigned long const &);
template char *
string_traits<long long>::into_buf(char *, char *, long long const &);
template char *string_traits<unsigned long long>::into_buf(
  char *, char *, unsigned long long const &);
} // namespace pqxx


namespace pqxx::internal
{
std::string demangle_type_name(char const raw[])
{
#if defined(PQXX_HAVE_CXA_DEMANGLE)
  // We've got __cxa_demangle.  Use it to get a friendlier type name.
  int status{0};

  // We've seen this fail on FreeBSD 11.3 (see #361).  Trying to throw a
  // meaningful exception only made things worse.  So in case of error, just
  // fall back to the raw name.
  //
  // When __cxa_demangle fails, it's guaranteed to return null.
  std::unique_ptr<char, void (*)(char *)> const demangled{
    abi::__cxa_demangle(raw, nullptr, nullptr, &status),
    // NOLINTNEXTLINE(*-no-malloc,cppcoreguidelines-owning-memory)
    [](char *x) { std::free(x); }};
#else
  std::unique_ptr<char> demangled{};
#endif
  return std::string{demangled ? demangled.get() : raw};
}


// TODO: Equivalents for converting a null in the other direction.
void PQXX_COLD throw_null_conversion(std::string const &type)
{
  throw conversion_error{concat("Attempt to convert SQL null to ", type, ".")};
}


void PQXX_COLD throw_null_conversion(std::string_view type)
{
  throw conversion_error{concat("Attempt to convert SQL null to ", type, ".")};
}


std::string PQXX_COLD state_buffer_overrun(int have_bytes, int need_bytes)
{
  // We convert these in standard library terms, not for the localisation
  // so much as to avoid "error cycles," if these values in turn should fail
  // to get enough buffer space.
  // C++20: Use formatting library.
  std::stringstream have, need;
  have << have_bytes;
  need << need_bytes;
  return "Have " + have.str() + " bytes, need " + need.str() + ".";
}
} // namespace pqxx::internal


namespace
{
template<typename TYPE> inline TYPE from_string_arithmetic(std::string_view in)
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

  auto const base{
    "Could not convert '" + std::string(in) +
    "' "
    "to " +
    pqxx::type_name<TYPE>};
  if (std::empty(msg))
    throw pqxx::conversion_error{base + "."};
  else
    throw pqxx::conversion_error{base + ": " + msg};
}
} // namespace


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace
{
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
template<typename T> class dumb_stringstream : public std::stringstream
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


template<typename F>
inline bool PQXX_COLD from_dumb_stringstream(
  dumb_stringstream<F> &s, F &result, std::string_view text)
{
  s.str(std::string{text});
  return static_cast<bool>(s >> result);
}


// These are hard, and some popular compilers still lack std::from_chars.
template<typename T>
inline T PQXX_COLD from_string_awful_float(std::string_view text)
{
  if (std::empty(text))
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
      if constexpr (have_thread_local)
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


namespace pqxx
{
/// Floating-point to_buf implemented in terms of to_string.
template<std::floating_point T>
zview string_traits<T>::to_buf(char *begin, char *end, T const &value)
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
      return "nan"_zv;
    if (std::isinf(value))
      return (value > 0) ? "infinity"_zv : "-infinity"_zv;
    auto text{to_string_float(value)};
    auto have{end - begin};
    auto need{std::size(text) + 1};
    if (need > std::size_t(have))
      throw conversion_error{
        "Could not convert floating-point number to string: "
        "buffer too small.  " +
        state_buffer_overrun(have, need)};
    text.copy(begin, need);
    return zview{begin, std::size(text)};
  }
#endif
}


template zview string_traits<float>::to_buf(char *, char *, float const &);
template zview string_traits<double>::to_buf(char *, char *, double const &);
template zview
string_traits<long double>::to_buf(char *, char *, long double const &);


template<std::floating_point T>
char *string_traits<T>::into_buf(char *begin, char *end, T const &value)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  return wrap_to_chars(begin, end, value);
#else
  return generic_into_buf(begin, end, value);
#endif
}


template char *string_traits<float>::into_buf(char *, char *, float const &);
template char *string_traits<double>::into_buf(char *, char *, double const &);
template char *
string_traits<long double>::into_buf(char *, char *, long double const &);


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
template<typename F>
inline std::string PQXX_COLD
to_dumb_stringstream(dumb_stringstream<F> &s, F value)
{
  s.str("");
  s << value;
  return s.str();
}
#endif
} // namespace pqxx


namespace pqxx::internal
{
/// Floating-point implementations for @c pqxx::to_string().
template<typename T> std::string to_string_float(T value)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  {
    static constexpr auto space{string_traits<T>::size_buffer(value)};
    std::string buf;
    buf.resize(space);
    std::string_view const view{
      string_traits<T>::to_buf(std::data(buf), std::data(buf) + space, value)};
    buf.resize(static_cast<std::size_t>(std::end(view) - std::begin(view)));
    return buf;
  }
#else
  {
    // In this rare case, we can convert to std::string but not to a simple
    // buffer.  So, implement to_buf in terms of to_string instead of the other
    // way around.
    if constexpr (have_thread_local)
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


namespace pqxx
{
template<pqxx::internal::integer T>
T string_traits<T>::from_string(std::string_view text)
{
  return from_string_arithmetic<T>(text);
}

template short string_traits<short>::from_string(std::string_view);
template unsigned short
  string_traits<unsigned short>::from_string(std::string_view);
template int string_traits<int>::from_string(std::string_view);
template unsigned string_traits<unsigned>::from_string(std::string_view);
template long string_traits<long>::from_string(std::string_view);
template unsigned long
  string_traits<unsigned long>::from_string(std::string_view);
template long long string_traits<long long>::from_string(std::string_view);
template unsigned long long
  string_traits<unsigned long long>::from_string(std::string_view);


template<std::floating_point T>
T string_traits<T>::from_string(std::string_view text)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)
  return from_string_arithmetic<T>(text);
#else
  return from_string_awful_float<T>(text);
#endif
}


template float string_traits<float>::from_string(std::string_view);
template double string_traits<double>::from_string(std::string_view);
template long double string_traits<long double>::from_string(std::string_view);
} // namespace pqxx


namespace pqxx::internal
{
template std::string to_string_float(float);
template std::string to_string_float(double);
template std::string to_string_float(long double);
} // namespace pqxx::internal


bool pqxx::string_traits<bool>::from_string(std::string_view text)
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
    throw conversion_error{
      "Failed conversion to bool: '" + std::string{text} + "'."};
}
