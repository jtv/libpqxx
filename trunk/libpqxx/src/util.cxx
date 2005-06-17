/*-------------------------------------------------------------------------
 *
 *   FILE
 *	util.cxx
 *
 *   DESCRIPTION
 *      Various utility functions for libpqxx
 *
 * Copyright (c) 2003-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#ifdef PQXX_HAVE_LOCALE
#include <locale>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <cerrno>
#include <new>
#include <sstream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "libpq-fe.h"

#include "pqxx/util"

using namespace PGSTD;
using namespace pqxx::internal;

namespace
{
// It turns out that NaNs are pretty hard to do portably.  If the appropriate
// C++ traits functions are not available, C99 defines a NAN macro (also widely
// supported in other dialects, I believe) and some functions that can do the
// same work.  But if none of those are available, we need to resort to
// compile-time "0/0" expressions.  Most compilers won't crash while compiling
// those anymore, but there may be unneeded warnings--which are almost as bad.
template<typename T> void set_to_NaN(T &);

#if defined(PQXX_HAVE_QUIET_NAN)
template<typename T> inline void set_to_NaN(T &t)
	{ t = numeric_limits<T>::quiet_NaN(); }
#elif defined(PQXX_HAVE_C_NAN)
template<typename T> inline void set_to_NaN(T &t) { t = NAN; }
#elif defined(PQXX_HAVE_NAN)
template<> inline void set_to_NaN(float &t) { t = fnan(""); }
template<> inline void set_to_NaN(double &t) { t = nan(""); }
#ifdef PQXX_HAVE_LONG_DOUBLE
template<> inline void set_to_NaN(long double &t) { t = lnan(""); }
#endif
#else
const float nan_f(0.0/0.0);
template<> inline void set_to_NaN(float &t) { t = nan_f; }
const double nan_d(0.0/0.0);
template<> inline void set_to_NaN(double &t) { t = nan_d; }
#ifdef PQXX_HAVE_LONG_DOUBLE
const long double nan_ld(0.0/0.0);
template<> inline void set_to_NaN(long double &t) { t = nan_ld; }
#endif

#endif

/// For use in string parsing: add new numeric digit to intermediate value
template<typename L, typename R>
  inline L absorb_digit(L value, R digit) throw ()
{
  return 10*value + digit;
}

} // namespace


namespace pqxx
{
template<> void from_string(const char Str[], long &Obj)
{
  int i = 0;
  long result = 0;

  if (!isdigit(Str[i]))
  {
    if (Str[i] != '-')
      throw runtime_error("Could not convert string to integer: '" +
      	string(Str) + "'");

    for (++i; isdigit(Str[i]); ++i)
    {
      const long newresult = absorb_digit(result, -digit_to_number(Str[i]));
      if (newresult > result)
        throw runtime_error("Integer too small to read: " + string(Str));
      result = newresult;
    }
  }
  else for (; isdigit(Str[i]); ++i)
  {
    const long newresult = absorb_digit(result, digit_to_number(Str[i]));
    if (newresult < result)
      throw runtime_error("Integer too large to read: " + string(Str));
    result = newresult;
  }

  if (Str[i])
    throw runtime_error("Unexpected text after integer: '" + string(Str) + "'");

  Obj = result;
}

template<> void from_string(const char Str[], unsigned long &Obj)
{
  int i = 0;
  unsigned long result;

  if (!Str) throw runtime_error("Attempt to convert NULL string to integer");

  if (!isdigit(Str[i]))
    throw runtime_error("Could not convert string to unsigned integer: '" +
    	string(Str) + "'");

  for (result=0; isdigit(Str[i]); ++i)
  {
    const unsigned long newres = absorb_digit(result, digit_to_number(Str[i]));
    if (newres < result)
      throw runtime_error("Unsigned integer too large to read: " + string(Str));

    result = newres;
  }

  if (Str[i])
    throw runtime_error("Unexpected text after integer: '" + string(Str) + "'");

  Obj = result;
}

} // namespace pqxx


namespace
{
template<typename T> inline void from_string_signed(const char Str[], T &Obj)
{
  long L;
  pqxx::from_string(Str, L);
  const T result = T(L);
  if (result != L) throw runtime_error("Overflow in integer conversion");
  Obj = result;
}

template<typename T> inline void from_string_unsigned(const char Str[], T &Obj)
{
  unsigned long L;
  pqxx::from_string(Str, L);
  const T result = T(L);
  if (result != L)
    throw runtime_error("Overflow in unsigned integer conversion");
  Obj = result;
}

/* These are hard.  Sacrifice performance of specialized, nonflexible,
 * non-localized code and lean on standard library.  Some special-case code
 * handles NaNs.
 */
template<typename T> inline void from_string_float(const char Str[], T &Obj)
{
  bool ok = false;
  T result;

  if (Str[0]=='N' || Str[0]=='n')
  {
    // Accept "NaN," "nan," etc.
    ok = ((Str[1]=='A'||Str[1]=='a') && (Str[2]=='N'||Str[2]=='n') && !Str[3]);
    set_to_NaN(result);
  }
  else
  {
    stringstream S(Str);
#ifdef PQXX_HAVE_LOCALE
    S.imbue(locale("C"));
#endif
    ok = (S >> result);
  }

  if (!ok)
    throw runtime_error("Could not convert string to numeric value: '" +
	string(Str) + "'");

  Obj = result;
}

} // namespace


namespace pqxx
{

template<> void from_string(const char Str[], int &Obj)
{
  from_string_signed(Str, Obj);
}

template<> void from_string(const char Str[], unsigned int &Obj)
{
  from_string_unsigned(Str, Obj);
}


template<> void from_string(const char Str[], short &Obj)
{
  from_string_signed(Str, Obj);
}

template<> void from_string(const char Str[], unsigned short &Obj)
{
  from_string_unsigned(Str, Obj);
}

template<> void from_string(const char Str[], float &Obj)
{
  float result;
  from_string_float(Str, result);
  Obj = result;
}

template<> void from_string(const char Str[], double &Obj)
{
  from_string_float(Str, Obj);
}

#if defined(PQXX_HAVE_LONG_DOUBLE)
template<> void from_string(const char Str[], long double &Obj)
{
  from_string_float(Str, Obj);
}
#endif


template<> void from_string(const char Str[], bool &Obj)
{
  if (!Str)
    throw runtime_error("Attempt to read NULL string");

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
      from_string(Str, I);
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
    throw invalid_argument("Failed conversion to bool: '" + string(Str) + "'");

  Obj = result;
}

} // namespace pqxx


namespace
{
template<typename T> inline string to_string_unsigned(T Obj)
{
  if (!Obj) return "0";

  char buf[4*sizeof(T)+1];
  char *p = &buf[sizeof(buf)];
  *--p = '\0';
  for (T next; Obj > 0; Obj = next)
  {
    next = Obj / 10;
    *--p = number_to_digit(Obj-next*10);
  }
  return p;
}

template<typename T> inline string to_string_fallback(T Obj)
{
  stringstream S;
#ifdef PQXX_HAVE_LOCALE
  S.imbue(locale("C"));
#endif
  S << Obj;
  string R;
  S >> R;
  return R;
}


template<typename T> inline bool is_NaN(T Obj)
{
    // TODO: Is this workaround for missing isnan() reliable?  Infinities?
  return 
#if defined(PQXX_HAVE_ISNAN)
    isnan(Obj);
#elif defined(PQXX_HAVE_LIMITS)
    !(Obj <= Obj+numeric_limits<T>::max());
#else
    !(Obj <= Obj + 1000);
#endif
}


template<typename T> inline string to_string_float(T Obj)
{
  // TODO: Omit this special case if NaN is output as "nan"/"NAN"/"NaN"
#ifndef PQXX_HAVE_NAN_OUTPUT
  if (is_NaN(Obj)) return "nan";
#endif
  return to_string_fallback(Obj);
}


template<typename T> inline string to_string_signed(T Obj)
{
  if (Obj < 0)
  {
    // Remember--the smallest negative number for a given two's-complement type
    // cannot be negated.
    if (-Obj > 0)
      return '-' + to_string_unsigned(-Obj);
    else
      return to_string_fallback(Obj);
  }

  return to_string_unsigned(Obj);
}

} // namespace


namespace pqxx
{
template<> string to_string(const short &Obj)
{
  return to_string_signed(Obj);
}

template<> string to_string(const unsigned short &Obj)
{
  return to_string_unsigned(Obj);
}

template<> string to_string(const int &Obj)
{
  return to_string_signed(Obj);
}

template<> string to_string(const unsigned int &Obj)
{
  return to_string_unsigned(Obj);
}

template<> string to_string(const long &Obj)
{
  return to_string_signed(Obj);
}

template<> string to_string(const unsigned long &Obj)
{
  return to_string_unsigned(Obj);
}

template<> string to_string(const float &Obj)
{
  return to_string_float(Obj);
}

template<> string to_string(const double &Obj)
{
  return to_string_float(Obj);
}

#if defined(PQXX_HAVE_LONG_DOUBLE)
template<> string to_string(const long double &Obj)
{
  return to_string_float(Obj);
}
#endif

template<> string to_string(const bool &Obj)
{
  return Obj ? "true" : "false";
}

template<> string to_string(const char &Obj)
{
  string s;
  s += Obj;
  return s;
}

} // namespace pqxx


void pqxx::internal::FromString_string(const char Str[], string &Obj)
{
  if (!Str)
    throw runtime_error("Attempt to convert NULL C string to C++ string");
  Obj = Str;
}


void pqxx::internal::FromString_ucharptr(const char Str[],
    const unsigned char *&Obj)
{
  const char *C;
  FromString(Str, C);
  Obj = reinterpret_cast<const unsigned char *>(C);
}


#ifdef PQXX_HAVE_PQESCAPESTRING
namespace
{
string libpq_escape(const char str[], size_t len)
{
  char *buf = 0;
  string result;

  try
  {
    /* Going by the letter of the PQescapeString() documentation we only need
     * 2*len+1 bytes.  But what happens to nonprintable characters?  They might
     * be escaped to octal notation, whether in current or future versions of
     * libpq--in which case we would need this more conservative size.
     */
    buf = new char[5*len + 1];
  }
  catch (const bad_alloc &)
  {
    /* Okay, maybe we're just dealing with an extremely large string.  Try a
     * more aggressive size limit, which is likely to be just fine.
     */
    buf = new char[2*len+1];
  }

  try
  {
    const size_t bytes = PQescapeString(buf, str, len);
    result.assign(buf, bytes);
  }
  catch (const exception &)
  {
    delete [] buf;
    throw;
  }
  delete [] buf;

  return result;
}
} // namespace
#endif

string pqxx::sqlesc(const char str[])
{
  string result;
#ifdef PQXX_HAVE_PQESCAPESTRING
  result = libpq_escape(str, strlen(str));
#else
  for (size_t i=0; str[i]; ++i)
  {
    if (isprint(str[i]))
    {
      switch (str[i])
      {
      case '\'':
      case '\\':
	result += str[i];
      }
      result += str[i];
    }
    else
    {
        char s[8];
        sprintf(s,
	        "\\%03o",
		static_cast<unsigned int>(static_cast<unsigned char>(str[i])));
        result.append(s, 4);
    }
  }
#endif
  return result;
}

string pqxx::sqlesc(const char str[], size_t len)
{
  string result;
#ifdef PQXX_HAVE_PQESCAPESTRING
  result = libpq_escape(str, len);
#else
  for (size_t i=0; (i < len) && str[i]; ++i)
  {
    if (isprint(str[i]))
    {
      switch (str[i])
      {
      case '\'':
      case '\\':
	result += str[i];
      }
      result += str[i];
    }
    else
    {
        char s[8];
        sprintf(s,
	        "\\%03o",
		static_cast<unsigned int>(static_cast<unsigned char>(str[i])));
        result.append(s, 4);
    }
  }
#endif

  return result;
}


string pqxx::sqlesc(const string &str)
{
  string result;
  const string::const_iterator str_end(str.end());
  for (string::const_iterator i = str.begin(); i != str_end; ++i)
  {
    if (isprint(*i) || isspace(*i))
    {
      switch (*i)
      {
      case '\'':
      case '\\':
	result += *i;
      }
      result += *i;
    }
    else
    {
        char s[8];
        sprintf(s,
	        "\\%03o",
		static_cast<unsigned int>(static_cast<unsigned char>(*i)));
        result.append(s, 4);
    }
  }
  return result;
}


string pqxx::internal::Quote_string(const string &Obj, bool EmptyIsNull)
{
  return (EmptyIsNull && Obj.empty()) ? "null" : ("'" + sqlesc(Obj) + "'");
}


string pqxx::internal::Quote_charptr(const char Obj[], bool EmptyIsNull)
{
  return Obj ? Quote(string(Obj), EmptyIsNull) : "null";
}


void pqxx::internal::freemem_result(pqxx::internal::pq::PGresult *p) throw ()
	{ PQclear(p); }

void pqxx::internal::freemem_notif(pqxx::internal::pq::PGnotify *p) throw ()
{
#ifdef PQXX_HAVE_PQFREENOTIFY
  PQfreeNotify(p);
#else
  freepqmem(p);
#endif
}


string pqxx::internal::namedclass::description() const
{
  try
  {
    string desc = classname();
    if (!name().empty()) desc += " '" + name() + "'";
    return desc;
  }
  catch (const exception &)
  {
    // Oops, string composition failed!  Probably out of memory.
    // Let's try something easier.
  }
  return name().empty() ? classname() : name();
}


void pqxx::internal::CheckUniqueRegistration(const namedclass *New,
    const namedclass *Old)
{
  if (!New)
    throw logic_error("libpqxx internal error: NULL pointer registered");
  if (Old)
  {
    if (Old == New)
      throw logic_error("Started " + New->description() + " twice");
    throw logic_error("Started " + New->description() + " "
		      "while " + Old->description() + " still active");
  }
}


void pqxx::internal::CheckUniqueUnregistration(const namedclass *New,
    const namedclass *Old)
{
  if (New != Old)
  {
    if (!New)
      throw logic_error("Expected to close " + Old->description() + ", "
	  		"but got NULL pointer instead");
    if (!Old)
      throw logic_error("Closed " + New->description() + ", "
	 		"which wasn't open");
    throw logic_error("Closed " + New->description() + "; "
		      "expected to close " + Old->description());
  }
}


void pqxx::internal::freepqmem(void *p)
{
#ifdef PQXX_HAVE_PQFREEMEM
  PQfreemem(p);
#else
  free(p);
#endif
}


void pqxx::internal::sleep_seconds(int s)
{
  if (s <= 0) return;

#if defined(PQXX_HAVE_SLEEP)
  // Use POSIX.1 sleep() if available
  sleep(s);
#elif defined(_WIN32)
  // Windows has its own Sleep(), which speaks milliseconds
  Sleep(s*1000);
#else
  // If all else fails, use select() on nothing and specify a timeout
  fd_set F;
  FD_ZERO(&F);
  struct timeval timeout;
  timeout.tv_sec = s;
  timeout.tv_usec = 0;
  if (select(0, &F, &F, &F, &timeout) == -1) switch (errno)
  {
  case EINVAL:	// Invalid timeout
	throw out_of_range("Invalid timeout value: " + to_string(s));
  case EINTR:	// Interrupted by signal
	break;
  case ENOMEM:	// Out of memory
	throw bad_alloc();
  default:
    throw logic_error("libpqxx internal error in select()");
  }
#endif
}


