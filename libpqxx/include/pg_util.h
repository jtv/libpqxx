/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_util.h
 *
 *   DESCRIPTION
 *      Various utility definitions for libpqxx
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_UTIL_H
#define PG_UTIL_H

#include <cstdio>
#include <stdexcept>
#include <string>
#include <typeinfo>

extern "C"
{
#include "postgres_fe.h"
#include "libpq-fe.h"
}

#include "pg_compiler.h"


namespace Pg
{
typedef long Result_size_type;
typedef int Tuple_size_type;

// C-style format strings for various built-in types.  Only allowed for
// certain types, for which this function is explicitly specialized below.
template<typename T> inline const char *FmtString(const T &);

// Not implemented to prevent accidents with irregular meaning of argument:
// template<> inline const char *FmtString(const char *const &) { return "%s"; }

template<> inline const char *FmtString(const int &)           { return  "%i"; }
template<> inline const char *FmtString(const long &)          { return "%li"; }
template<> inline const char *FmtString(const unsigned &)      { return  "%u"; }
template<> inline const char *FmtString(const unsigned long &) { return "%lu"; }
template<> inline const char *FmtString(const float &)         { return  "%f"; }
template<> inline const char *FmtString(const double &)        { return "%lf"; }
template<> inline const char *FmtString(const char &)          { return  "%c"; }


// Convert object of built-in type to string
template<typename T> inline PGSTD::string ToString(const T &Obj)
{
  // TODO: Find a decent way to determine max string length at compile time!
  char Buf[500];
  sprintf(Buf, FmtString(Obj), Obj);
  return PGSTD::string(Buf);
}

template<> inline PGSTD::string ToString(const PGSTD::string &Obj) {return Obj;}
template<> inline PGSTD::string ToString(const char *const &Obj) { return Obj; }


template<typename T> inline void FromString(const char Str[], T &Obj)
{
  if (!Str) throw PGSTD::runtime_error("Attempt to convert NULL string to " +
		                     PGSTD::string(typeid(T).name()));

  if (sscanf(Str, FmtString(Obj), &Obj) != 1)
    throw PGSTD::runtime_error("Cannot convert value '" + 
		             PGSTD::string(Str) + 
			     "' to " + typeid(T).name());
}


template<> inline void FromString(const char Str[], PGSTD::string &Obj)
{
  if (!Str) 
    throw PGSTD::runtime_error("Attempt to convert NULL C string to C++ string");
  Obj = Str;
}


template<> inline void FromString(const char Str[], const char *&Obj)
{
  if (!Str)
    throw PGSTD::runtime_error("Attempt to read NULL string");
  Obj = Str;
}


// Generate SQL-quoted version of string.  If EmptyIsNull is set, an empty
// string will generate the null value rather than an empty string.
template<typename T> PGSTD::string Quote(const T &Obj, bool EmptyIsNull=false);


// std::string version, on which the other versions are built
template<> inline PGSTD::string Quote(const PGSTD::string &Obj, 
		                      bool EmptyIsNull)
{
    if (EmptyIsNull && Obj.empty())
        return "null";

    PGSTD::string Result = "'" + Obj + "'";

    for (PGSTD::string::size_type i = Result.size()-2; i > 0; --i)
        if ((Result[i] == '\'') ||
	    (Result[i] == '\\'))
	    Result.insert(i, 1, Result[i]);

    return Result;
}


// In the special case of const char *, the null pointer is represented as
// the null value.
template<> inline PGSTD::string Quote(const char *const & Obj, bool EmptyIsNull)
{
  if (!Obj) return "null";
  return Quote(PGSTD::string(Obj), EmptyIsNull);
}


// Generic version
template<typename T> inline PGSTD::string Quote(const T &Obj, bool EmptyIsNull)
{
  return Quote(ToString(Obj), EmptyIsNull);
}



// Return user-readable name for a class.  Specialize this whereever used.
template<typename T> PGSTD::string Classname(const T *);


// Ensure proper opening/closing of GUEST objects related to a "host" object,
// where only a single GUEST may exist for a single host at any given time.
// The template assumes that Classname<GUEST>() returns a user-readable name
// for the guest type, and that GUEST::Name() returns a user-readable name for
// the GUEST object.
template<typename GUEST>
class Unique
{
public:
  Unique() : m_Guest(0) {}

  const GUEST *get() const throw () { return m_Guest; }

  void Register(const GUEST *G)
  {
    if (!G) throw PGSTD::logic_error("Internal libpqxx error: NULL " + 
		                     Classname(G));
    
    if (m_Guest)
    {
      if (G == m_Guest)
        throw PGSTD::logic_error(Classname(G) +
			         " '" +
			         G->Name() +
			         "' started more than once without closing");

      throw PGSTD::logic_error("Started " + 
		               Classname(G) +
			       " '" + 
			       G->Name() + 
			       "' while '" +
			       m_Guest->Name() +
			       "' was still active");
    }
    
    m_Guest = G;
  }

  void Unregister(const GUEST *G)
  {
    if (G != m_Guest)
    {
      if (!G) 
	throw PGSTD::logic_error("Closing NULL " + Classname(G));
      else if (!m_Guest)
	throw PGSTD::logic_error("Closing " + 
			         Classname(G) +
			         " '" +
			         G->Name() +
			         "' which wasn't open");
      else
	throw PGSTD::logic_error("Closing wrong " + 
			         Classname(G) +
			         "; expected '" +
			         m_Guest->Name() +
			         "' but got '" +
			         G->Name() +
			         "'");
    }

    m_Guest = 0;
  }

private:
  const GUEST *m_Guest;

  // Not allowed:
  Unique(const Unique &);
  Unique &operator=(const Unique &);
};


const Result_size_type Result_size_type_min =
  PGSTD::numeric_limits<Result_size_type>::min();
const Result_size_type Result_size_type_max =
  PGSTD::numeric_limits<Result_size_type>::max();

}

#endif

