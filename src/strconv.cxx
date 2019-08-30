/** Implementation of string conversions.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <locale>
#include <cstdlib>
#include <string_view>
#include <system_error>

#if defined(PQXX_HAVE_CXA_DEMANGLE)
#include <cxxabi.h>
#endif

#include "pqxx/except"
#include "pqxx/strconv"


namespace
{
/// String comparison between string_view.
inline bool equal(std::string_view lhs, std::string_view rhs)
{
  return lhs.compare(rhs) == 0;
}
} // namespace


namespace pqxx::internal
{
std::string demangle_type_name(const char raw[])
{
#if defined(PQXX_HAVE_CXA_DEMANGLE)
  int status = 0;
  std::unique_ptr<char, std::function<void(char *)>> name{
	abi::__cxa_demangle(raw, nullptr, nullptr, &status),
	[](char *x){ std::free(x); }};
  if (status != 0)
    throw std::runtime_error(
	std::string{"Could not demangle type name '"} + name.get() + "': "
	"__cxa_demangle failed.");
  return std::string{name.get()};
#else
  return std::string{raw};
#endif // PQXX_HAVE_CXA_DEMANGLE
}

void throw_null_conversion(const std::string &type)
{
  throw conversion_error{"Attempt to convert null to " + type + "."};
}


std::string state_buffer_overrun(ptrdiff_t have_bytes, ptrdiff_t need_bytes)
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


#if defined(PQXX_HAVE_CHARCONV_INT) || defined(PQXX_HAVE_CHARCONV_FLOAT)
template<typename TYPE>
TYPE pqxx::internal::builtin_traits<TYPE>::from_string(std::string_view in)
{
  const char *end = in.data() + in.size();
  TYPE out;
  const auto res = std::from_chars(in.data(), end, out);
  if (res.ec == std::errc() and res.ptr == end) return out;

  std::string msg;
  if (res.ec == std::errc())
  {
    msg = "Could not parse full string.";
  }
  else switch (res.ec)
  {
  case std::errc::result_out_of_range:
    msg = "Value out of range.";
    break;
  case std::errc::invalid_argument:
    msg = "Invalid argument.";
    break;
  default:
    break;
  }

  const std::string base =
	"Could not convert '" + std::string(in) + "' "
	"to " + pqxx::type_name<TYPE>;
  if (msg.empty()) throw pqxx::conversion_error{base + "."};
  else throw pqxx::conversion_error{base + ": " + msg};
}
#endif


#if !defined(PQXX_HAVE_CHARCONV_INT)
namespace
{
[[noreturn]] void report_overflow()
{
  throw pqxx::conversion_error{
	"Could not convert string to integer: value out of range."};
}


/// Return 10*n, or throw exception if it overflows.
template<typename T> inline T safe_multiply_by_ten(T n)
{
  using limits = std::numeric_limits<T>;

  constexpr T ten{10};
  constexpr T high_threshold(std::numeric_limits<T>::max() / ten);
  if (n > high_threshold) report_overflow();
  if constexpr (limits::is_signed)
  {
    constexpr T low_threshold(std::numeric_limits<T>::min() / ten);
    if (low_threshold > n) report_overflow();
  }
  return T(n * ten);
}


/// Add digit d to nonnegative n, or throw exception if it overflows.
template<typename T> inline T safe_add_digit(T n, T d)
{
  const T high_threshold{static_cast<T>(std::numeric_limits<T>::max() - d)};
  if (n > high_threshold) report_overflow();
  return static_cast<T>(n + d);
}


/// Subtract digit d to nonpositive n, or throw exception if it overflows.
template<typename T> inline T safe_sub_digit(T n, T d)
{
  const T low_threshold{static_cast<T>(std::numeric_limits<T>::min() + d)};
  if (n < low_threshold) report_overflow();
  return static_cast<T>(n - d);
}


/// For use in string parsing: add new numeric digit to intermediate value.
template<typename L, typename R>
  inline L absorb_digit_positive(L value, R digit)
{
  return safe_add_digit(safe_multiply_by_ten(value), L(digit));
}


/// For use in string parsing: subtract digit from intermediate value.
template<typename L, typename R>
  inline L absorb_digit_negative(L value, R digit)
{
  return safe_sub_digit(safe_multiply_by_ten(value), L(digit));
}


/// Compute numeric value of given textual digit (assuming that it is a digit).
[[maybe_unused]] constexpr int digit_to_number(char c) noexcept
{ return c - '0'; }


template<typename T> T from_string_signed(std::string_view str)
{
  int i = 0;
  T result = 0;

  if (isdigit(str.data()[i]))
  {
    for (; isdigit(str.data()[i]); ++i)
      result = absorb_digit_positive(result, digit_to_number(str.data()[i]));
  }
  else
  {
    if (str.data()[i] != '-')
      throw pqxx::conversion_error{
        "Could not convert string to integer: '" + std::string{str} + "'."};

    for (++i; isdigit(str.data()[i]); ++i)
      result = absorb_digit_negative(result, digit_to_number(str.data()[i]));
  }

  if (str.data()[i])
    throw pqxx::conversion_error{
      "Unexpected text after integer: '" + std::string{str} + "'."};

  return result;
}

template<typename T> T from_string_unsigned(std::string_view str)
{
  int i = 0;
  T result = 0;

  if (not isdigit(str.data()[i]))
    throw pqxx::conversion_error{
      "Could not convert string to unsigned integer: '" +
      std::string{str} + "'."};

  for (; isdigit(str.data()[i]); ++i)
    result = absorb_digit_positive(result, digit_to_number(str.data()[i]));

  if (str.data()[i])
    throw pqxx::conversion_error{
      "Unexpected text after integer: '" + std::string{str} + "'."};

  return result;
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_INT


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace
{
bool valid_infinity_string(std::string_view str) noexcept
{
  return
	equal("infinity", str) or
	equal("Infinity", str) or
	equal("INFINITY", str) or
	equal("inf", str);
}
} // namespace
#endif


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


template<typename T> inline void set_to_Inf(T &t, int sign=1)
{
  T value = std::numeric_limits<T>::infinity();
  if (sign < 0) value = -value;
  t = value;
}


// These are hard, and popular compilers do not yet implement std::from_chars.
template<typename T> inline T from_string_float(std::string_view str)
{
  bool ok = false;
  T result;

  switch (str[0])
  {
  case 'N':
  case 'n':
    // Accept "NaN," "nan," etc.
    ok = (
      (str[1]=='A' or str[1]=='a') and
      (str[2]=='N' or str[2]=='n') and
      (str[3] == '\0'));
    result = std::numeric_limits<T>::quiet_NaN();
    break;

  case 'I':
  case 'i':
    ok = valid_infinity_string(str);
    set_to_Inf(result);
    break;

  default:
    if (str[0] == '-' and valid_infinity_string(&str[1]))
    {
      ok = true;
      set_to_Inf(result, -1);
    }
    else
    {
      thread_local dumb_stringstream<T> S;
      // Visual Studio 2017 seems to fail on repeated conversions if the
      // clear() is done before the seekg().  Still don't know why!  See #124
      // and #125.
      S.seekg(0);
      S.clear();
      S.str(std::string{str});
      ok = static_cast<bool>(S >> result);
    }
    break;
  }

  if (not ok)
    throw pqxx::conversion_error{
      "Could not convert string to numeric value: '" +
      std::string{str} + "'."};

  return result;
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_FLOAT


namespace pqxx::internal
{
#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
/// Fallback floating-point implementation of @c pqxx::to_string().
/** In this rare case, a @c std::string is easier to produce than a
 * @c std::string_view.  So, we implement @c to_buf in terms of @c to_string
 * instead of the other way around.
 */
template<typename T> PQXX_LIBEXPORT std::string to_string_float(T value)
{
  thread_local dumb_stringstream<T> s;
  s.str("");
  s << value;
  return s.str();
}


template std::string to_string_float(float);
template std::string to_string_float(double);
template std::string to_string_float(long double);
#endif // !PQXX_HAVE_CHARCONV_FLOAT


/// Floating-point to_buf implemented in terms of to_string.
template<typename T>
std::string_view to_buf_float(char *begin, char *end, T value)
{
#if defined(PQXX_HAVE_CHARCONV_FLOAT)

  // Implement using std::to_chars.
  const auto res = std::to_chars<T>(begin, end - 1, value);
  if (res.ec != std::errc()) switch (res.ec)
  {
  case std::errc::value_too_large:
    throw conversion_overrun{
	std::string{"Could not convert "} + type_name<T> + " to string: "
	"buffer too small (" + to_string(end - begin) + " bytes)."};
  default:
    throw conversion_error{
	std::string{"Could not convert "} + type_name<T> + " to string."};
  }

  *res.ptr = '\0';
  return std::string_view{begin, std::size_t(res.ptr - begin)};

#else

  // Implement it ourselves.  Weird detail: since this workaround is based on
  // std::stringstream, which produces a std::string, it's actually easier to
  // build the to_buf() on top of the to_string() than the other way around.
  if (std::isnan(value)) return "nan";
  if (std::isinf(value)) return (value > 0) ? "infinity" : "-infinity";
  auto text = to_string_float(value);
  auto have = end - begin;
  auto need = text.size() + 1;
  if (need > std::size_t(have))
    throw conversion_error{
	"Could not convert floating-point number to string: "
	"buffer too small.  " +
        state_buffer_overrun(have, std::ptrdiff_t(need))
	};
  std::memcpy(begin, text.c_str(), need);
  return std::string_view{begin, text.size()};

#endif // !PQXX_HAVE_CHARCONV_FLOAT
}


template std::string_view to_buf_float(char *, char *, float);
template std::string_view to_buf_float(char *, char *, double);
template std::string_view to_buf_float(char *, char *, long double);
} // namespace pqxx::internal


#if defined(PQXX_HAVE_CHARCONV_INT)
namespace pqxx::internal
{
template short
builtin_traits<short>::from_string(std::string_view);
template unsigned short
builtin_traits<unsigned short>::from_string(std::string_view);
template int
builtin_traits<int>::from_string(std::string_view);
template unsigned int
builtin_traits<unsigned int>::from_string(std::string_view);
template long
builtin_traits<long>::from_string(std::string_view);
template unsigned long
builtin_traits<unsigned long>::from_string(std::string_view);
template long long
builtin_traits<long long>::from_string(std::string_view);
template unsigned long long
builtin_traits<unsigned long long>::from_string(std::string_view);
} // namespace pqxx
#endif // PQXX_HAVE_CHARCONV_INT


#if !defined(PQXX_HAVE_CHARCONV_INT)
namespace pqxx::internal
{
template<> short
builtin_traits<short>::from_string(std::string_view str)
	{ return from_string_signed<short>(str); }
template<> unsigned short
builtin_traits<unsigned short>::from_string(std::string_view str)
	{ return from_string_unsigned<unsigned short>(str); }
template<> int
builtin_traits<int>::from_string(std::string_view str)
	{ return from_string_signed<int>(str); }
template<> unsigned int
builtin_traits<unsigned int>::from_string(std::string_view str)
	{ return from_string_unsigned<unsigned int>(str); }
template<> long
builtin_traits<long>::from_string(std::string_view str)
	{ return from_string_signed<long>(str); }
template<>
unsigned long builtin_traits<unsigned long>::from_string(std::string_view str)
	{ return from_string_unsigned<unsigned long>(str); }
template<> long long
builtin_traits<long long>::from_string(std::string_view str)
	{ return from_string_signed<long long>(str); }
template<> unsigned long long
builtin_traits<unsigned long long>::from_string(std::string_view str)
	{ return from_string_unsigned<unsigned long long>(str); }
} // namespace pqxx::internal
#endif // !PQXX_HAVE_CHARCONV_INT


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace pqxx::internal
{
template<> float
builtin_traits<float>::from_string(std::string_view str)
	{ return from_string_float<float>(str); }
template<> double
builtin_traits<double>::from_string(std::string_view str)
	{ return from_string_float<double>(str); }
template<> long double
builtin_traits<long double>::from_string(std::string_view str)
	{ return from_string_float<long double>(str); }
} // namespace pqxx::internal
#endif // !PQXX_HAVE_CHARCONV_FLOAT


namespace pqxx::internal
{
template<> bool builtin_traits<bool>::from_string(std::string_view str)
{
  bool OK, result;

  switch (str.size())
  {
  case 0:
    result = false;
    OK = true;
    break;

  case 1:
    switch (str[0])
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

    default:
      OK = false;
      break;
    }
    break;

  case 4:
    result = true;
    OK = (equal(str, "true") or equal(str, "TRUE"));
    break;

  case 5:
    result = false;
    OK = (equal(str, "false") or equal(str, "FALSE"));
    break;

  default:
    OK = false;
    break;
  }

  if (not OK)
    throw conversion_error{
      "Failed conversion to bool: '" + std::string{str} + "'."};

  return result;
}
} // namespace pqxx::internal
