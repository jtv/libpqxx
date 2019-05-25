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
#include <limits>
#include <locale>
#include <string_view>
#include <system_error>

#if defined(PQXX_HAVE_CHARCONV_INT) || defined(PQXX_HAVE_CHARCONV_FLOAT)
#include <charconv>
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
void throw_null_conversion(const std::string &type)
{
  throw conversion_error{"Attempt to convert null to " + type + "."};
}
} // namespace pqxx::internal


#if defined(PQXX_HAVE_CHARCONV_INT) || defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace
{
template<typename T> void wrap_from_chars(std::string_view in, T &out)
{
  const char *end = in.data() + in.size();
  const auto res = std::from_chars(in.data(), end, out);
  if (res.ec == std::errc() and res.ptr == end) return;

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
	"to " + pqxx::type_name<T>;
  if (msg.empty()) throw pqxx::conversion_error{base + "."};
  else throw pqxx::conversion_error{base + ": " + msg};
}


/// How big of a buffer do we want for representing a T?
template<typename T> constexpr int size_buffer()
{
  using lim = std::numeric_limits<T>;
  // Allocate room for how many digits?  There's "max_digits10" for
  // floating-point numbers, but only "digits10" for integer types.
  constexpr auto digits = std::max({lim::digits10, lim::max_digits10});
  // Leave a little bit of extra room for signs, decimal points, and the like.
  return digits + 4;
}


/// Call @c std::to_chars.  It differs for integer vs. floating-point types.
template<typename TYPE, bool INTEGRAL> struct to_chars_caller;

#if defined(PQXX_HAVE_CHARCONV_INT)
/// For integer types, we pass "base 10" to @c std::to_chars.
template<typename TYPE> struct to_chars_caller<TYPE, true>
{
  static std::to_chars_result call(char *begin, char *end, TYPE in)
	{ return std::to_chars(begin, end, in, 10); }
};
#endif

#if defined(PQXX_HAVE_CHARCONV_FLOAT)
/// For floating-point types, we pass "general format" to @c std::to_chars.
template<typename TYPE>
template<typename TYPE> struct to_chars_caller<TYPE, true>
{
  static std::to_chars_result call(char *begin, char *end, TYPE in)
	{ return std::to_chars(begin, end, in, std::chars_format::general); }
};
#endif
} // namespace


namespace pqxx::internal
{
template<typename T> std::string builtin_traits<T>::to_string(T in)
{
  char buf[size_buffer<T>()];

  // Annoying: we need to make slightly different calls to std::to_chars
  // depending on whether this is an integral type or a floating-point type.
  // Use to_chars_caller to hide the difference.
  constexpr bool is_integer = std::numeric_limits<T>::is_integer;
  const auto res = to_chars_caller<T, is_integer>::call(
	buf, buf + sizeof(buf), in);
  if (res.ec == std::errc()) return std::string(buf, res.ptr);

  std::string msg;
  switch (res.ec)
  {
  case std::errc::value_too_large:
    msg = "Value too large.";
    break;
  default:
    break;
  }

  const std::string base =
    std::string{"Could not convert "} + type_name<T> + " to string";
  if (msg.empty()) throw pqxx::conversion_error{base + "."};
  else throw pqxx::conversion_error{base + ": " + msg};
}


/// Translate @c from_string calls to @c wrap_from_chars calls.
/** The only difference is the type of the string.
 */
template<typename TYPE>
void builtin_traits<TYPE>::from_string(std::string_view str, TYPE &obj)
	{ wrap_from_chars(std::string_view{str}, obj); }
} // namespace pqxx::internal
#endif // PQXX_HAVE_CHARCONV_INT || PQXX_HAVE_CHARCONV_FLOAT


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace
{
template<typename T> inline void set_to_Inf(T &t, int sign=1)
{
  T value = std::numeric_limits<T>::infinity();
  if (sign < 0) value = -value;
  t = value;
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_FLOAT


#if !defined(PQXX_HAVE_CHARCONV_INT)
namespace
{
[[noreturn]] void report_overflow()
{
  throw pqxx::conversion_error{
	"Could not convert string to integer: value out of range."};
}


/** Helper to check for underflow before multiplying a number by 10.
 *
 * Needed just so the compiler doesn't get to complain about an "if (n < 0)"
 * clause that's pointless for unsigned numbers.
 */
template<typename T, bool is_signed> struct underflow_check;

/* Specialization for signed types: check.
 */
template<typename T> struct underflow_check<T, true>
{
  static void check_before_adding_digit(T n)
  {
    constexpr T ten{10};
    if (n < 0 and (std::numeric_limits<T>::min() / ten) > n) report_overflow();
  }
};

/* Specialization for unsigned types: no check needed becaue negative
 * numbers don't exist.
 */
template<typename T> struct underflow_check<T, false>
{
  static void check_before_adding_digit(T) {}
};


/// Return 10*n, or throw exception if it overflows.
template<typename T> T safe_multiply_by_ten(T n)
{
  using limits = std::numeric_limits<T>;
  constexpr T ten{10};
  if (n > 0 and (limits::max() / n) < ten) report_overflow();
  underflow_check<T, limits::is_signed>::check_before_adding_digit(n);
  return T(n * ten);
}


/// Add a digit d to n, or throw exception if it overflows.
template<typename T> T safe_add_digit(T n, T d)
{
  assert((n >= 0 and d >= 0) or (n <=0 and d <= 0));
  if ((n > 0) and (n > (std::numeric_limits<T>::max() - d))) report_overflow();
  if ((n < 0) and (n < (std::numeric_limits<T>::min() - d))) report_overflow();
  return n + d;
}


/// For use in string parsing: add new numeric digit to intermediate value
template<typename L, typename R>
  inline L absorb_digit(L value, R digit)
{
  return L(safe_multiply_by_ten(value) + L(digit));
}


template<typename T> void from_string_signed(std::string_view str, T &obj)
{
  int i = 0;
  T result = 0;

  if (not isdigit(str[i]))
  {
    if (str[i] != '-')
      throw pqxx::conversion_error{
        "Could not convert string to integer: '" + std::string{str} + "'."};

    for (++i; isdigit(str[i]); ++i)
      result = absorb_digit(result, -pqxx::internal::digit_to_number(str[i]));
  }
  else
  {
    for (; isdigit(str[i]); ++i)
      result = absorb_digit(result, pqxx::internal::digit_to_number(str[i]));
  }

  if (str[i])
    throw pqxx::conversion_error{
      "Unexpected text after integer: '" + std::string{str} + "'."};

  obj = result;
}

template<typename T> void from_string_unsigned(std::string_view str, T &obj)
{
  int i = 0;
  T result = 0;

  if (not isdigit(str[i]))
    throw pqxx::conversion_error{
      "Could not convert string to unsigned integer: '" +
      std::string{str} + "'."};

  for (; isdigit(str[i]); ++i)
    result = absorb_digit(result, pqxx::internal::digit_to_number(str[i]));

  if (str[i])
    throw pqxx::conversion_error{
      "Unexpected text after integer: '" + std::string{str} + "'."};

  obj = result;
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


/// Wrapper for std::stringstream with C locale.
/** Some of our string conversions use the standard library.  But, they must
 * _not_ obey the system's locale settings, or a value like 1000.0 might end
 * up looking like "1.000,0".
 *
 * Initialising the stream (including locale and tweaked precision) seems to
 * be expensive though.  So, create thread-local instances which we re-use.
 * It's a lockless way of keeping global variables thread-safe, basically.
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


/* These are hard.  Sacrifice performance of specialized, nonflexible,
 * non-localized code and lean on standard library.  Some special-case code
 * handles NaNs.
 */
template<typename T> inline void from_string_float(
	std::string_view str,
	T &obj)
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
      // TODO: Inefficient.  Can we get good std::from_chars implementations?
      S.str(std::string{str});
      ok = static_cast<bool>(S >> result);
    }
    break;
  }

  if (not ok)
    throw pqxx::conversion_error{
      "Could not convert string to numeric value: '" +
      std::string{str} + "'."};

  obj = result;
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_FLOAT


#if !defined(PQXX_HAVE_CHARCONV_INT)
namespace
{
template<typename T> inline std::string to_string_unsigned(T obj)
{
  if (not obj) return "0";

  // Every byte of width on T adds somewhere between 3 and 4 digits to the
  // maximum length of our decimal string.
  char buf[4*sizeof(T)+1];

  char *p = &buf[sizeof(buf)];
  *--p = '\0';
  while (obj > 0)
  {
    *--p = pqxx::internal::number_to_digit(int(obj % 10));
    obj = T(obj / 10);
  }
  return p;
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_INT


#if !defined(PQXX_HAVE_CHARCONV_INT) || !defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace
{
template<typename T> inline std::string to_string_fallback(T obj)
{
  thread_local dumb_stringstream<T> s;
  s.str("");
  s << obj;
  return s.str();
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_INT || !PQXX_HAVE_CHARCONV_FLOAT


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace
{
template<typename T> inline std::string to_string_float(T obj)
{
  if (std::isnan(obj)) return "nan";
  if (std::isinf(obj)) return (obj > 0) ? "infinity" : "-infinity";
  return to_string_fallback(obj);
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_FLOAT


#if !defined(PQXX_HAVE_CHARCONV_INT)
namespace
{
template<typename T> inline std::string to_string_signed(T obj)
{
  if (obj < 0)
  {
    // Remember--the smallest negative number for a given two's-complement type
    // cannot be negated.
    const bool negatable = (obj != std::numeric_limits<T>::min());
    if (negatable)
      return '-' + to_string_unsigned(-obj);
    else
      return to_string_fallback(obj);
  }

  return to_string_unsigned(obj);
}
} // namespace
#endif // !PQXX_HAVE_CHARCONV_INT


#if defined(PQXX_HAVE_CHARCONV_INT)
namespace pqxx::internal
{
template void
builtin_traits<short>::from_string(std::string_view, short &);
template void
builtin_traits<unsigned short>::from_string(std::string_view, unsigned short &);
template void
builtin_traits<int>::from_string(std::string_view, int &);
template void
builtin_traits<unsigned int>::from_string(std::string_view, unsigned int &);
template void
builtin_traits<long>::from_string(std::string_view, long &);
template void
builtin_traits<unsigned long>::from_string(std::string_view, unsigned long &);
template void
builtin_traits<long long>::from_string(std::string_view, long long &);
template void
builtin_traits<unsigned long long>::from_string(
	std::string_view, unsigned long long &);
} // namespace pqxx
#endif // PQXX_HAVE_CHARCONV_INT


#if defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace pqxx
{
template
void string_traits<float>::from_string(std::string_view str, float &obj);
template
void string_traits<double>::from_string(std::string_view str, double &obj);
template
void string_traits<long double>::from_string(
	std::string_view str,
	long double &obj);
} // namespace pqxx
#endif // PQXX_HAVE_CHARCONV_FLOAT


#if defined(PQXX_HAVE_CHARCONV_INT)
namespace pqxx::internal
{
template
std::string builtin_traits<short>::to_string(short obj);
template
std::string builtin_traits<unsigned short>::to_string(unsigned short obj);
template
std::string builtin_traits<int>::to_string(int obj);
template
std::string builtin_traits<unsigned int>::to_string(unsigned int obj);
template
std::string builtin_traits<long>::to_string(long obj);
template
std::string builtin_traits<unsigned long>::to_string(unsigned long obj);
template
std::string builtin_traits<long long>::to_string(long long obj);
template
std::string builtin_traits<unsigned long long>::to_string(
	unsigned long long obj);
} // namespace pqxx::internal
#endif // PQXX_HAVE_CHARCONV_INT


#if defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace pqxx::internal
{
template
std::string builtin_traits<float>::to_string(float obj);
template
std::string builtin_traits<double>::to_string(double obj);
template
std::string builtin_traits<long double>::to_string(long double obj);
} // namespace pqxx::internal
#endif // PQXX_HAVE_CHARCONV_FLOAT


#if !defined(PQXX_HAVE_CHARCONV_INT)
namespace pqxx::internal
{
template<>
void builtin_traits<short>::from_string(std::string_view str, short &obj)
	{ from_string_signed(str, obj); }
template<>
std::string builtin_traits<short>::to_string(short obj)
	{ return to_string_signed(obj); }
template<>
void builtin_traits<unsigned short>::from_string(
	std::string_view str,
	unsigned short &obj)
	{ from_string_unsigned(str, obj); }
template<>
std::string builtin_traits<unsigned short>::to_string(unsigned short obj)
	{ return to_string_unsigned(obj); }
template<>
void builtin_traits<int>::from_string(std::string_view str, int &obj)
	{ from_string_signed(str, obj); }
template<>
std::string builtin_traits<int>::to_string(int obj)
	{ return to_string_signed(obj); }
template<>
void builtin_traits<unsigned int>::from_string(
	std::string_view str,
	unsigned int &obj)
	{ from_string_unsigned(str, obj); }
template<>
std::string builtin_traits<unsigned int>::to_string(unsigned int obj)
	{ return to_string_unsigned(obj); }
template<>
void builtin_traits<long>::from_string(std::string_view str, long &obj)
	{ from_string_signed(str, obj); }
template<>
std::string builtin_traits<long>::to_string(long obj)
	{ return to_string_signed(obj); }
template<>
void builtin_traits<unsigned long>::from_string(
	std::string_view str,
	unsigned long &obj)
	{ from_string_unsigned(str, obj); }
template<>
std::string builtin_traits<unsigned long>::to_string(unsigned long obj)
	{ return to_string_unsigned(obj); }
template<>
void builtin_traits<long long>::from_string(
	std::string_view str,
	long long &obj)
	{ from_string_signed(str, obj); }
template<>
std::string builtin_traits<long long>::to_string(long long obj)
	{ return to_string_signed(obj); }
template<>
void builtin_traits<unsigned long long>::from_string(
	std::string_view str,
	unsigned long long &obj)
	{ from_string_unsigned(str, obj); }
template<>
std::string builtin_traits<unsigned long long>::to_string(
        unsigned long long obj)
	{ return to_string_unsigned(obj); }
} // namespace pqxx::internal
#endif // !PQXX_HAVE_CHARCONV_INT


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
namespace pqxx::internal
{
template<>
void builtin_traits<float>::from_string(std::string_view str, float &obj)
	{ from_string_float(str, obj); }
template<>
std::string builtin_traits<float>::to_string(float obj)
	{ return to_string_float(obj); }
template<>
void builtin_traits<double>::from_string(std::string_view str, double &obj)
	{ from_string_float(str, obj); }
template<>
std::string builtin_traits<double>::to_string(double obj)
	{ return to_string_float(obj); }
template<>
void builtin_traits<long double>::from_string(
	std::string_view str, long double &obj)
	{ from_string_float(str, obj); }
template<>
std::string builtin_traits<long double>::to_string(long double obj)
	{ return to_string_float(obj); }
} // namespace pqxx::internal
#endif // !PQXX_HAVE_CHARCONV_FLOAT


namespace pqxx::internal
{
template<> void builtin_traits<bool>::from_string(
	std::string_view str,
	bool &obj)
{
  bool OK, result=false;

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
  }

  if (not OK)
    throw conversion_error{
      "Failed conversion to bool: '" + std::string{str} + "'."};

  obj = result;
}


template<> std::string builtin_traits<bool>::to_string(bool obj)
{
  return obj ? "true" : "false";
}
} // namespace pqxx::internal
