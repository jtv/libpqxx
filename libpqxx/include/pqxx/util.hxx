/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/util.hxx
 *
 *   DESCRIPTION
 *      Various utility definitions for libpqxx
 *      DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/util instead.
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#include <cstdio>
#include <cctype>
#include <stdexcept>
#include <string>
#include <typeinfo>

extern "C"
{
#include "libpq-fe.h"
}


namespace pqxx
{
typedef long result_size_type;
typedef int tuple_size_type;

/// PostgreSQL row identifier type
typedef Oid oid;

/// The "null" oid
const oid oid_none = InvalidOid;


/// C-style format strings for various built-in types.  Only allowed for
/// certain types, for which this function is explicitly specialized below.
template<typename T> inline const char *FmtString(T);

// Not implemented to prevent accidents with irregular meaning of argument:
// template<> inline const char *FmtString(const char *&) { return "%s"; }

template<> inline const char *FmtString(short)         { return "%hd"; }
template<> inline const char *FmtString(unsigned short){ return "%hu"; }
template<> inline const char *FmtString(int)           { return  "%i"; }
template<> inline const char *FmtString(long)          { return "%li"; }
template<> inline const char *FmtString(unsigned)      { return  "%u"; }
template<> inline const char *FmtString(unsigned long) { return "%lu"; }
template<> inline const char *FmtString(float)         { return  "%f"; }
template<> inline const char *FmtString(double)        { return "%lf"; }
template<> inline const char *FmtString(long double)   { return "%Lf"; }
template<> inline const char *FmtString(char)          { return  "%c"; }
template<> inline const char *FmtString(unsigned char) { return  "%c"; }


/// Convert object of built-in type to string
/** Caution: the conversion is done using the currently active locale, whereas
 * PostgreSQL expects values in the "default" (C) locale.  This means that if
 * you intend to use this function from a locale that renders the data types in
 * question (particularly numeric types like float and int) differently from the
 * C default, you'll need to switch back to the C locale before the call.
 * This should be fixed at some point in the future.
 */
template<typename T> inline PGSTD::string ToString(const T &Obj)
{
  // TODO: Find a decent way to determine max string length at compile time!
  char Buf[500];
  sprintf(Buf, FmtString(Obj), Obj);
  return PGSTD::string(Buf);
}

// TODO: Implement date conversions

template<> inline PGSTD::string ToString(const PGSTD::string &Obj) {return Obj;}
template<> inline PGSTD::string ToString(const char *const &Obj) { return Obj; }
template<> inline PGSTD::string ToString(char *const &Obj) { return Obj; }

template<> inline PGSTD::string ToString(const bool &Obj) 
{ 
  return ToString(unsigned(Obj));
}

template<> inline PGSTD::string ToString(const short &Obj)
{
  return ToString(int(Obj));
}

template<> inline PGSTD::string ToString(const unsigned short &Obj)
{
  return ToString(unsigned(Obj));
}


/// Convert string to object of built-in type
/** Caution: the conversion is done using the currently active locale, whereas
 * PostgreSQL delivers values in the "default" (C) locale.  This means that if
 * you intend to use this function from a locale that doesn't understand the 
 * data types in question (particularly numeric types like float and int) in
 * default C format, you'll need to switch back to the C locale before the call.
 * This should be fixed at some point in the future.
 */
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
    throw PGSTD::runtime_error("Attempt to convert NULL C string to C++ "
	                       "string");
  Obj = Str;
}


template<> inline void FromString(const char Str[], const char *&Obj)
{
  if (!Str)
    throw PGSTD::runtime_error("Attempt to read NULL string");
  Obj = Str;
}

template<> inline void FromString(const char Str[], bool &Obj)
{
  if (!Str)
    throw PGSTD::runtime_error("Attempt to read NULL string");

  switch (Str[0])
  {
  case 0:
  case 'f':
    Obj = false;
    break;
  case '0':
    {
      int I;
      FromString(Str, I);
      Obj = (I != 0);
    }
    break;
  default:
    Obj = true;
  }
}


/// Quote string for use in SQL
/** Generate SQL-quoted version of string.  If EmptyIsNull is set, an empty
 * string will generate the null value rather than an empty string.
 */
template<typename T> inline PGSTD::string Quote(const T &Obj, bool EmptyIsNull);

/// std::string version, on which the other versions are built
template<> inline PGSTD::string Quote(const PGSTD::string &Obj, 
		                      bool EmptyIsNull)
{
  if (EmptyIsNull && Obj.empty()) return "null";

  PGSTD::string Result;
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


/// In the special case of const char *, the null pointer is represented as
/// the null value.
template<> inline PGSTD::string Quote(const char *const & Obj, 
		                      bool EmptyIsNull)
{
  if (!Obj) return "null";
  return Quote(PGSTD::string(Obj), EmptyIsNull);
}


/// Specialization for string constants
/** This specialization is a little complicated, because string constants are
 * of the type char[], not of type const char * as one might expect.  Note that
 * the size of the array is part of the type, for which reason we need it in our
 * template here.
 */
template<int LEN> inline PGSTD::string Quote(const char (&Obj)[LEN],
    					     bool EmptyIsNull)		//[t18]
{
  return Quote(PGSTD::string(Obj), EmptyIsNull);
}


/// Generic version: generate SQL-quoted version of object.  If EmptyIsNull is
/// set, an empty string will generate the null value rather than an empty 
/// string.
template<typename T> inline PGSTD::string Quote(const T &Obj, bool EmptyIsNull)
{
  return Quote(ToString(Obj), EmptyIsNull);
}


/// Quote string for use in SQL
/** This version of the function never generates null values.
 */
template<typename T> inline PGSTD::string Quote(T Obj)
{
  return Quote(Obj, false);
}


/// Return user-readable name for a class.  Specialize this whereever used.
template<typename T> PGSTD::string Classname(const T *);


/// Keep track of a libpq-allocated pointer to be free()d automatically.
/** Ownership policy is simple: object dies when PQAlloc object's value does.
 * If the available PostgreSQL development files supply PQfreemem() or 
 * PQfreeNotify(), this is used to free the memory.  If not, free() is used 
 * instead.  This matters on Windows, where memory allocated by a DLL must be 
 * freed by the same DLL.
 */
template<typename T> class PQAlloc
{
  T *m_Obj;
public:
  PQAlloc() : m_Obj(0) {}

  /// Assume ownership of a pointer
  explicit PQAlloc(T *obj) : m_Obj(obj) {}

  ~PQAlloc() { close(); }

  /// Assume ownership of a pointer, freeing the previous one (if any)
  /** If the new and the old pointer are identical, no action is performed.
   */
  PQAlloc &operator=(T *obj) throw ()
  { 
    if (obj != m_Obj)
    {
      close();
      m_Obj = obj;
    }
    return *this;
  }

  /// Is this pointer non-null?
  operator bool() const throw () { return m_Obj != 0; }

  /// Is this pointer null?
  bool operator!() const throw () { return !m_Obj; }

  /// Dereference pointer
  /** Throws a logic_error if the pointer is null.
   */
  T *operator->() const throw (PGSTD::logic_error)
  {
    if (!m_Obj) throw PGSTD::logic_error("Null pointer dereferenced");
    return m_Obj;
  }

  /// Dereference pointer
  /** Throws a logic_error if the pointer is null.
   */
  T &operator*() const throw (PGSTD::logic_error) { return *operator->(); }

  /// Obtain underlying pointer
  /** Ownership of the pointer's memory remains with the PQAlloc object
   */
  T *c_ptr() const throw () { return m_Obj; }

  /// Free and reset current pointer (if any)
  void close() throw () { if (m_Obj) freemem(); m_Obj = 0; }

private:
  void freemem() throw ()
  {
#if defined(PQXX_HAVE_PQFREEMEM)
    PQfreemem(reinterpret_cast<unsigned char *>(m_Obj));
#else
    free(m_Obj);
#endif
  }

  PQAlloc(const PQAlloc &);		// Not allowed
  PQAlloc &operator=(const PQAlloc &);	// Not allowed
};


/// Special version for notify structures, using PQfreeNotify() if available
template<> inline void PQAlloc<PGnotify>::freemem() throw ()
{
#if defined(PQXX_HAVE_PQFREEMEM)
    PQfreemem(reinterpret_cast<unsigned char *>(m_Obj));
#elif defined(PQXX_HAVE_PQFREENOTIFY)
    PQfreeNotify(m_Obj);
#else
    free(m_Obj);
#endif
}



/// Ensure proper opening/closing of GUEST objects related to a "host" object
/** Only a single GUEST may exist for a single host at any given time.  The 
 * template assumes that Classname<GUEST>() returns a user-readable name for the
 * guest type, and that GUEST::name() returns a user-readable name for the GUEST
 * object.
 *
 * This template used to be called Unique.
 */
template<typename GUEST>
class unique
{
public:
  unique() : m_Guest(0) {}

  GUEST *get() const throw () { return m_Guest; }

  void Register(GUEST *G)
  {
    if (!G) throw PGSTD::logic_error("Internal libpqxx error: NULL " + 
		                     Classname(G));
    
    if (m_Guest)
    {
      if (G == m_Guest)
        throw PGSTD::logic_error(Classname(G) +
			         " '" +
			         G->name() +
			         "' started more than once without closing");

      throw PGSTD::logic_error("Started " + 
		               Classname(G) +
			       " '" + 
			       G->name() + 
			       "' while '" +
			       m_Guest->name() +
			       "' was still active");
    }
    
    m_Guest = G;
  }

  void Unregister(GUEST *G)
  {
    if (G != m_Guest)
    {
      if (!G) 
	throw PGSTD::logic_error("Closing NULL " + Classname(G));
      else if (!m_Guest)
	throw PGSTD::logic_error("Closing " + 
			         Classname(G) +
			         " '" +
			         G->name() +
			         "' which wasn't open");
      else
	throw PGSTD::logic_error("Closing wrong " + 
			         Classname(G) +
			         "; expected '" +
			         m_Guest->name() +
			         "' but got '" +
			         G->name() +
			         "'");
    }

    m_Guest = 0;
  }

private:
  GUEST *m_Guest;

  // Not allowed:
  unique(const unique &);
  unique &operator=(const unique &);
};

}


