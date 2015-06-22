/*-------------------------------------------------------------------------
 *
 *   FILE
 *	strconv.cxx
 *
 *   DESCRIPTION
 *      implementation of string conversions
 *
 * Copyright (c) 2008-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <cmath>
#include <cstring>
#include <limits>
#include <locale>

#include "pqxx/except"
#include "pqxx/strconv"


using namespace pqxx::internal;


namespace
{
template<typename T> inline void set_to_NaN(T &t)
{
  t = std::numeric_limits<T>::quiet_NaN();
}

template<typename T> inline void set_to_Inf(T &t, int sign=1)
{
  T value = std::numeric_limits<T>::infinity();
  if (sign < 0) value = -value;
  t = value;
}


void report_overflow()
{
  throw pqxx::failure(
	"Could not convert string to integer: value out of range.");
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
    const T ten(10);
    if (n < 0 && (std::numeric_limits<T>::min() / ten) > n) report_overflow();
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
  typedef std::numeric_limits<T> limits;
  const T ten(10);
  if (n > 0 && (limits::max() / n) < ten) report_overflow();
  underflow_check<T, limits::is_signed>::check_before_adding_digit(n);
  return T(n * ten);
}


/// Add a digit d to n, or throw exception if it overflows.
template<typename T> T safe_add_digit(T n, T d)
{
  assert((n >= 0 && d >= 0) || (n <=0 && d <= 0));
  if ((n > 0) && (n > (std::numeric_limits<T>::max() - d))) report_overflow();
  if ((n < 0) && (n < (std::numeric_limits<T>::min() - d))) report_overflow();
  return n + d;
}


/// For use in string parsing: add new numeric digit to intermediate value
template<typename L, typename R>
  inline L absorb_digit(L value, R digit)
{
  return L(safe_multiply_by_ten(value) + L(digit));
}


template<typename T> void from_string_signed(const char Str[], T &Obj)
{
  int i = 0;
  T result = 0;

  if (!isdigit(Str[i]))
  {
    if (Str[i] != '-')
      throw pqxx::failure(
        "Could not convert string to integer: '" + std::string(Str) + "'");

    for (++i; isdigit(Str[i]); ++i)
      result = absorb_digit(result, -digit_to_number(Str[i]));
  }
  else for (; isdigit(Str[i]); ++i)
    result = absorb_digit(result, digit_to_number(Str[i]));

  if (Str[i])
    throw pqxx::failure(
      "Unexpected text after integer: '" + std::string(Str) + "'");

  Obj = result;
}

template<typename T> void from_string_unsigned(const char Str[], T &Obj)
{
  int i = 0;
  T result = 0;

  if (!isdigit(Str[i]))
    throw pqxx::failure(
      "Could not convert string to unsigned integer: '" +
      std::string(Str) + "'");

  for (; isdigit(Str[i]); ++i)
    result = absorb_digit(result, digit_to_number(Str[i]));

  if (Str[i])
    throw pqxx::failure(
      "Unexpected text after integer: '" + std::string(Str) + "'");

  Obj = result;
}


bool valid_infinity_string(const char str[])
{
  // TODO: Also accept less sensible case variations.
  return
	strcmp("infinity", str) == 0 ||
	strcmp("Infinity", str) == 0 ||
	strcmp("INFINITY", str) == 0 ||
	strcmp("inf", str) == 0;
}


/* These are hard.  Sacrifice performance of specialized, nonflexible,
 * non-localized code and lean on standard library.  Some special-case code
 * handles NaNs.
 */
template<typename T> inline void from_string_float(const char Str[], T &Obj)
{
  bool ok = false;
  T result;

  switch (Str[0])
  {
  case 'N':
  case 'n':
    // Accept "NaN," "nan," etc.
    ok = ((Str[1]=='A'||Str[1]=='a') && (Str[2]=='N'||Str[2]=='n') && !Str[3]);
    set_to_NaN(result);
    break;

  case 'I':
  case 'i':
    ok = valid_infinity_string(Str);
    set_to_Inf(result);
    break;

  default:
    if (Str[0] == '-' && valid_infinity_string(&Str[1]))
    {
      ok = true;
      set_to_Inf(result, -1);
    }
    else
    {
      std::stringstream S(Str);
      S.imbue(std::locale("C"));
      ok = static_cast<bool>(S >> result);
    }
    break;
  }

  if (!ok)
    throw pqxx::failure(
      "Could not convert string to numeric value: '" +
      std::string(Str) + "'");

  Obj = result;
}

template<typename T> inline std::string to_string_unsigned(T Obj)
{
  if (!Obj) return "0";

  // Every byte of width on T adds somewhere between 3 and 4 digits to the
  // maximum length of our decimal string.
  char buf[4*sizeof(T)+1];

  char *p = &buf[sizeof(buf)];
  *--p = '\0';
  while (Obj > 0)
  {
    *--p = number_to_digit(int(Obj%10));
    Obj /= 10;
  }
  return p;
}

template<typename T> inline std::string to_string_fallback(T Obj)
{
  std::stringstream S;
  S.imbue(std::locale("C"));

  // Kirit reports getting two more digits of precision than
  // numeric_limits::digits10 would give him, so we try not to make him lose
  // those last few bits.
  S.precision(std::numeric_limits<T>::digits10 + 2);
  S << Obj;
  return S.str();
}


template<typename T> inline bool is_NaN(T Obj)
{
  return
#if defined(PQXX_HAVE_STD_ISNAN)
    std::isnan(Obj);
#elif defined(PQXX_HAVE_ISNAN)
    isnan(Obj);
#else
    !(Obj <= Obj + std::numeric_limits<T>::max());
#endif
}


template<typename T> inline bool is_Inf(T Obj)
{
  return
#if defined(PQXX_HAVE_STD_ISINF)
    std::isinf(Obj);
#elif defined(PQXX_HAVE_ISINF)
    isinf(Obj);
#else
    Obj >= Obj+10 && Obj <= 2*Obj && Obj >= 2*Obj;
#endif
}


template<typename T> inline std::string to_string_float(T Obj)
{
  if (is_NaN(Obj)) return "nan";
  if (is_Inf(Obj)) return Obj > 0 ? "infinity" : "-infinity";
  return to_string_fallback(Obj);
}


template<typename T> inline std::string to_string_signed(T Obj)
{
  if (Obj < 0)
  {
    // Remember--the smallest negative number for a given two's-complement type
    // cannot be negated.
    const bool negatable = (Obj != std::numeric_limits<T>::min());
    if (negatable)
      return '-' + to_string_unsigned(-Obj);
    else
      return to_string_fallback(Obj);
  }

  return to_string_unsigned(Obj);
}

} // namespace


namespace pqxx
{

namespace internal
{
void throw_null_conversion(const std::string &type)
{
  throw conversion_error("Attempt to convert null to " + type);
}
} // namespace pqxx::internal

void string_traits<bool>::from_string(const char Str[], bool &Obj)
{
  bool OK, result=false;

  switch (Str[0])
  {
  case 0:
    result = false;
    OK = true;
    break;

  case 'f':
  case 'F':
    result = false;
    OK = !(Str[1] &&
	   (strcmp(Str+1, "alse") != 0) &&
	   (strcmp(Str+1, "ALSE") != 0));
    break;

  case '0':
    {
      int I;
      string_traits<int>::from_string(Str, I);
      result = (I != 0);
      OK = ((I == 0) || (I == 1));
    }
    break;

  case '1':
    result = true;
    OK = !Str[1];
    break;

  case 't':
  case 'T':
    result = true;
    OK = !(Str[1] &&
	   (strcmp(Str+1, "rue") != 0) &&
	   (strcmp(Str+1, "RUE") != 0));
    break;

  default:
    OK = false;
  }

  if (!OK)
    throw argument_error(
      "Failed conversion to bool: '" + std::string(Str) + "'");

  Obj = result;
}

std::string string_traits<bool>::to_string(bool Obj)
{
  return Obj ? "true" : "false";
}

void string_traits<short>::from_string(const char Str[], short &Obj)
{
  from_string_signed(Str, Obj);
}

std::string string_traits<short>::to_string(short Obj)
{
  return to_string_signed(Obj);
}

void string_traits<unsigned short>::from_string(
	const char Str[],
	unsigned short &Obj)
{
  from_string_unsigned(Str, Obj);
}

std::string string_traits<unsigned short>::to_string(unsigned short Obj)
{
  return to_string_unsigned(Obj);
}

void string_traits<int>::from_string(const char Str[], int &Obj)
{
  from_string_signed(Str, Obj);
}

std::string string_traits<int>::to_string(int Obj)
{
  return to_string_signed(Obj);
}

void string_traits<unsigned int>::from_string(
	const char Str[],
	unsigned int &Obj)
{
  from_string_unsigned(Str, Obj);
}

std::string string_traits<unsigned int>::to_string(unsigned int Obj)
{
  return to_string_unsigned(Obj);
}

void string_traits<long>::from_string(const char Str[], long &Obj)
{
  from_string_signed(Str, Obj);
}

std::string string_traits<long>::to_string(long Obj)
{
  return to_string_signed(Obj);
}

void string_traits<unsigned long>::from_string(
	const char Str[],
	unsigned long &Obj)
{
  from_string_unsigned(Str, Obj);
}

std::string string_traits<unsigned long>::to_string(unsigned long Obj)
{
  return to_string_unsigned(Obj);
}

void string_traits<long long>::from_string(const char Str[], long long &Obj)
{
  from_string_signed(Str, Obj);
}

std::string string_traits<long long>::to_string(long long Obj)
{
  return to_string_signed(Obj);
}

void string_traits<unsigned long long>::from_string(
	const char Str[],
	unsigned long long &Obj)
{
  from_string_unsigned(Str, Obj);
}

std::string string_traits<unsigned long long>::to_string(
        unsigned long long Obj)
{
  return to_string_unsigned(Obj);
}

void string_traits<float>::from_string(const char Str[], float &Obj)
{
  from_string_float(Str, Obj);
}

std::string string_traits<float>::to_string(float Obj)
{
  return to_string_float(Obj);
}

void string_traits<double>::from_string(const char Str[], double &Obj)
{
  from_string_float(Str, Obj);
}

std::string string_traits<double>::to_string(double Obj)
{
  return to_string_float(Obj);
}

void string_traits<long double>::from_string(const char Str[], long double &Obj)
{
  from_string_float(Str, Obj);
}

std::string string_traits<long double>::to_string(long double Obj)
{
  return to_string_float(Obj);
}

} // namespace pqxx
