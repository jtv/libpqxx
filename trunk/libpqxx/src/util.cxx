/*-------------------------------------------------------------------------
 *
 *   FILE
 *	util.cxx
 *
 *   DESCRIPTION
 *      Various utility functions for libpqxx
 *
 * Copyright (c) 2003-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <locale>
#include <new>
#include <sstream>

#include "pqxx/util"

using namespace PGSTD;


namespace pqxx
{

template<> void from_string(const char Str[], long &Obj)
{
  const char *p = Str;
  bool neg = false;
  if (!isdigit(*p))
  {
    if (*p == '-')
    {
      neg = true;
      p++;
    }
    else
    {
      throw runtime_error("Could not convert string to integer: '" +
      	string(Str) + "'");
    }
  }

  long result;
  for (result=0; isdigit(*p); ++p)
  {
    const long newresult = 10*result + (*p-'0');
    if (newresult < result)
      throw runtime_error("Integer too large to read: " + string(Str));

    result = newresult;
  }

  if (*p)
    throw runtime_error("Unexpected text after integer: '" + string(Str) + "'");

  Obj = (neg ? -result : result);
}

template<> void from_string(const char Str[], unsigned long &Obj)
{
  if (!Str) throw runtime_error("Attempt to convert NULL string to integer");
  
  const char *p = Str;
  if (!isdigit(*p))
  {
    throw runtime_error("Could not convert string to unsigned integer: '" +
    	string(Str) + "'");
  }

  unsigned long result;
  for (result=0; isdigit(*p); ++p)
  {
    const unsigned long newresult = 10*result + (*p-'0');
    if (newresult < result)
      throw runtime_error("Unsigned integer too large to read: " + string(Str));

    result = newresult;
  }

  if (*p)
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

// These are hard.  Sacrifice performance and lean on standard library.
template<typename T> inline void from_string_float(const char Str[], T &Obj)
{
  locale Cloc("C");
  stringstream S(Str);
  S.imbue(Cloc);
  T result;
  if (!(S >> result))
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

template<> void from_string(const char Str[], long double &Obj)
{
  from_string_float(Str, Obj);
}


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


#include<cassert>// DEBUG CODE
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
assert(next < Obj);	// DEBUG CODE
    char c = ('0' + Obj - (next*10));
assert(isdigit(c));	// DEBUG CODE
    *--p = c;
assert(p > buf);	// DEBUG CODE
  }
  return p;
}

template<typename T> inline string to_string_fallback(T Obj)
{
  stringstream S;
  S << Obj;
  string R;
  S >> R;
  return R;
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
  return to_string_fallback(Obj);
}

template<> string to_string(const double &Obj)
{
  return to_string_fallback(Obj);
}

template<> string to_string(const long double &Obj)
{
  return to_string_fallback(Obj);
}

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


string pqxx::internal::Quote_string(const string &Obj, bool EmptyIsNull)
{
  if (EmptyIsNull && Obj.empty()) return "null";

  string Result;
  Result.reserve(Obj.size() + 2);
  Result += "'";

#ifdef PQXX_HAVE_PQESCAPESTRING

  char *const Buf = new char[2*Obj.size() + 1];
  try
  {
    PQescapeString(Buf, Obj.c_str(), Obj.size());
    Result += Buf;
  }
  catch (const PGSTD::exception &)
  {
    delete [] Buf;
    throw;
  }
  delete [] Buf;

#else

  for (PGSTD::string::size_type i=0; i < Obj.size(); ++i)
  {
    if (isgraph(Obj[i]))
    {
      switch (Obj[i])
      {
      case '\'':
      case '\\':
	Result += '\\';
      }
      Result += Obj[i];
    }
    else
    {
        char s[10];
        sprintf(s, 
	        "\\%03o", 
		static_cast<unsigned int>(static_cast<unsigned char>(Obj[i])));
        Result.append(s, 4);
    }
  }

#endif

  return Result + '\'';
}


string pqxx::internal::Quote_charptr(const char Obj[], bool EmptyIsNull)
{
  if (!Obj) return "null";
  return Quote(string(Obj), EmptyIsNull);
}


string pqxx::internal::namedclass::description() const throw ()
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


void pqxx::internal::freenotif(PGnotify *p)
{
#ifdef PQXX_HAVE_PQFREENOTIFY
  PQfreeNotify(p);
#else
  freepqmem(p);
#endif
}

