/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/util.hxx
 *
 *   DESCRIPTION
 *      Various utility definitions for libpqxx
 *      DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/util instead.
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"
#include "pqxx/config-public-libpq.h"

#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>


/// The home of all libpqxx classes, functions, templates, etc.
namespace pqxx
{
}


#ifdef PQXX_PQ_IN_NAMESPACE
// We want libpq in the pqxx::internal::pq namespace

namespace pqxx
{
namespace internal
{
namespace pq
{
#define PQXXPQ pqxx::internal::pq
extern "C"
{
#include "libpq-fe.h"
}
} // namespace pq
} // namespace internal
} // namespace pqxx

#else	// PQXX_PQ_IN_NAMESPACE
// We want libpq in the global namespace, with duplicates in pqxx::internal::pq

extern "C"
{
#include "libpq-fe.h"
}

namespace pqxx
{
namespace internal
{
namespace pq
{
#define PQXXPQ
typedef PQXXPQ::PGconn PGconn;
typedef PQXXPQ::PGresult PGresult;

} // namespace pq
} // namespace internal
} // namespace pqxx

#endif	// PQXX_PQ_IN_NAMESPACE


namespace pqxx
{
typedef long result_size_type;
typedef int tuple_size_type;

/// PostgreSQL row identifier type
typedef PQXXPQ::Oid oid;

/// The "null" oid
const oid oid_none = 0;


/// Dummy name, used by libpqxx in deliberate link errors
/** If you get an error involving this function while building your program, 
 * this means that the program contains an unsupported string conversion.
 *
 * In other words, the program tries to convert an object to a string, or a
 * string to an object, of a type for which libpqxx does not implement this
 * conversion.  A notable example is "long long," which is supported by many
 * compilers but does not exist in Standard C++.  
 *
 * In the case of "long long" and similar types, if your implementation of the
 * standard C++ library supports it, you may use a stringstream to perform the
 * conversion.  For other types, you may have to write the conversion routine
 * yourself.
 */
template<typename T> void error_unsupported_type_in_string_conversion(T);


/// Dummy name, used to generate meaningful link errors
/** If you get a link error naming this function, this means that your program
 * includes a string conversion involving a signed or unsigned char type.  The
 * problem with such conversions is that they are ambiguous: do you want to 
 * treat the char type as a small numeric type, or as a character type like 
 * plain char?
 */
template<typename T> PGSTD::string error_ambiguous_string_conversion(T);



// TODO: Implement date conversions

/// Attempt to convert postgres-generated string to given built-in type
/** If the form of the value found in the string does not match the expected 
 * type, e.g. if a decimal point is found when converting to an integer type,
 * the conversion fails.  Overflows (e.g. converting "9999999999" to a 16-bit
 * C++ type) are also treated as errors.
 * Only the simplest possible conversions are supported.  No fancy features
 * such as hexadecimal or octal, spurious signs, or exponent notation will work.
 * No whitespace is stripped away.  Only the kinds of strings that come out of 
 * PostgreSQL and out of to_string() can be converted.
 */
template<typename T> void from_string(const char Str[], T &Obj);

template<> void from_string(const char Str[], long &);			//[t45]
template<> void from_string(const char Str[], unsigned long &);		//[t45]
template<> void from_string(const char Str[], int &);			//[t45]
template<> void from_string(const char Str[], unsigned int &);		//[t45]
template<> void from_string(const char Str[], short &);			//[t45]
template<> void from_string(const char Str[], unsigned short &);	//[t45]
template<> void from_string(const char Str[], float &);			//[t46]
template<> void from_string(const char Str[], double &);		//[t46]
template<> void from_string(const char Str[], long double &);		//[t46]
template<> void from_string(const char Str[], bool &);			//[t76]

template<> inline void from_string(const char Str[],PGSTD::string &Obj)	//[t46]
	{ Obj = Str; }

template<> 
  inline void from_string(const char Str[], PGSTD::stringstream &Obj)	//[t0]
  	{ Obj.clear(); Obj << Str; }

template<typename T> 
  inline void from_string(const PGSTD::string &Str, T &Obj) 		//[t45]
	{ from_string(Str.c_str(), Obj); }

template<typename T>
  inline void from_string(const PGSTD::stringstream &Str, T &Obj)	//[t0]
  	{ from_string(Str.str(), Obj); }

template<> inline void 
from_string(const PGSTD::string &Str, PGSTD::string &Obj) 		//[t46]
	{ Obj = Str; }

template<> inline void
from_string(const PGSTD::string &, const char &Obj)
	{ error_ambiguous_string_conversion(Obj); }
template<> inline void
from_string(const PGSTD::string &, const signed char &Obj)
	{ error_ambiguous_string_conversion(Obj); }
template<> inline void
from_string(const PGSTD::string &, const unsigned char &Obj)
	{ error_ambiguous_string_conversion(Obj); }


/// Convert built-in type to a readable string that PostgreSQL will understand
/** No special formatting is done, and any locale settings are ignored.  The
 * resulting string will be human-readable and in a format suitable for use in
 * SQL queries.
 */
template<typename T> PGSTD::string to_string(const T &);

template<> PGSTD::string to_string(const short &);			//[t76]
template<> PGSTD::string to_string(const unsigned short &);		//[t76]
template<> PGSTD::string to_string(const int &);			//[t10]
template<> PGSTD::string to_string(const unsigned int &);		//[t13]
template<> PGSTD::string to_string(const long &);			//[t18]
template<> PGSTD::string to_string(const unsigned long &);		//[t20]
template<> PGSTD::string to_string(const float &);			//[t74]
template<> PGSTD::string to_string(const double &);			//[t74]
template<> PGSTD::string to_string(const long double &);		//[t74]
template<> PGSTD::string to_string(const bool &);			//[t76]

inline PGSTD::string to_string(const char Obj[]) 			//[t14]
	{ return PGSTD::string(Obj); }

inline PGSTD::string to_string(const PGSTD::stringstream &Obj)		//[t0]
	{ return Obj.str(); }

inline PGSTD::string to_string(const PGSTD::string &Obj) {return Obj;}	//[t21]

template<> PGSTD::string to_string(const char &);			//[t21]


template<> inline PGSTD::string to_string(const signed char &Obj)
	{ return error_ambiguous_string_conversion(Obj); }
template<> inline PGSTD::string to_string(const unsigned char &Obj)
	{ return error_ambiguous_string_conversion(Obj); }


/// Container of items with easy contents initialization and string rendering
/** Designed as a wrapper around an arbitrary container type, this class lets
 * you easily create a container object and provide its contents in the same
 * line.  Regular addition methods such as push_back() will also still work, but
 * you can now write things like: "items<int> numbers; numbers(1)(2)(3)(4);"
 *
 * Up to five elements may be specified directly as constructor arguments, e.g.
 * "items<int> numbers(1,2,3,4);"
 *
 * One thing that cannot be done with this simple class is create const objects
 * with nontrivial contents.  This is because the function invocation operator 
 * (which is being used to add items) modifies the container rather than
 * creating a new one.  This was done to keep performance within reasonable
 * bounds.
 *
 * @warning This class may see substantial change in its interface before it
 * stabilizes.  Do not count on it remaining the way it is.
 */
template<typename T=PGSTD::string, typename CONT=PGSTD::vector<T> >
class items : public CONT
{
public:
  /// Create empty items list
  items() : CONT() {}							//[t80]
  /// Create items list with one element
  explicit items(const T &t) : CONT() { push_back(t); }			//[]
  items(const T &t1, const T &t2) : CONT() 				//[t80]
  	{ push_back(t1); push_back(t2); }
  items(const T &t1, const T &t2, const T &t3) : CONT() 		//[]
  	{ push_back(t1); push_back(t2); push_back(t3); }
  items(const T &t1, const T &t2, const T &t3, const T &t4) : CONT() 	//[]
  	{ push_back(t1); push_back(t2); push_back(t3); push_back(t4); }
  items(const T&t1,const T&t2,const T&t3,const T&t4,const T&t5):CONT() 	//[]
  	{push_back(t1);push_back(t2);push_back(t3);push_back(t4);push_back(t5);}
  /// Copy container
  items(const CONT &c) : CONT(c) {}					//[]

  /// Add element to items list
  items &operator()(const T &t)						//[t80]
  {
    push_back(t);
    return *this;
  }
};


// TODO: Generalize--add transformation functor
/// Render items list as a string, using given separator between items
template<typename ITER> inline
PGSTD::string separated_list(const PGSTD::string &sep,
    ITER begin,
    ITER end)								//[t8]
{
  PGSTD::string result;
  if (begin != end)
  {
    result = to_string(*begin);
    for (++begin; begin != end; ++begin)
    {
      result += sep;
      result += to_string(*begin);
    }
  }
  return result;
}

/// Render items in a container as a string, using given separator
template<typename CONTAINER> inline
PGSTD::string separated_list(const PGSTD::string &sep,
    const CONTAINER &c)							//[t10]
{
  return separated_list(sep, c.begin(), c.end());
}


/// Private namespace for libpqxx's internal use; do not access
/** This namespace hides definitions internal to libpqxx.  These are not 
 * supposed to be used by client programs, and they may change at any time 
 * without notice.  
 * @warning Here be dragons!
 */
namespace internal
{
/// C-style format strings for various built-in types
/** @deprecated To be removed when ToString and FromString are taken out
 *
 * Only allowed for certain types, for which this function is explicitly 
 * specialized.  When attempting to use the template for an unsupported type,
 * the generic version will be instantiated.  This will result in a link error
 * for the symbol error_unsupported_type_in_string_conversion(), with a template
 * argument describing the unsupported input type.
 */
template<typename T> inline const char *FmtString(T t)
{
  error_unsupported_type_in_string_conversion(t);
  return 0;
}

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

} // namespace internal

/// Convert object of built-in type to string
/** @deprecated Use the newer, rewritten to_string instead.
 * @warning The conversion is done using the currently active locale, whereas
 * PostgreSQL expects values in the "default" (C) locale.  This means that if
 * you intend to use this function from a locale that renders the data types in
 * question (particularly numeric types like float and int) differently from the
 * C default, you'll need to switch back to the C locale before the call.
 * This problem does not exist with the newer to_string function template.
 */
template<typename T> inline PGSTD::string ToString(const T &Obj)
{
  // TODO: Find a decent way to determine max string length at compile time!
  char Buf[500];
  sprintf(Buf, internal::FmtString(Obj), Obj);
  return PGSTD::string(Buf);
}


template<> inline PGSTD::string ToString(const PGSTD::string &Obj) {return Obj;}
template<> inline PGSTD::string ToString(const char *const &Obj) { return Obj; }
template<> inline PGSTD::string ToString(char *const &Obj) { return Obj; }

template<> inline PGSTD::string ToString(const unsigned char *const &Obj)
{
  return reinterpret_cast<const char *>(Obj);
}

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
/** @deprecated Use the stricter, safer from_string instead.
 * @warning The conversion is done using the currently active locale, whereas
 * PostgreSQL delivers values in the "default" (C) locale.  This means that if
 * you intend to use this function from a locale that doesn't understand the 
 * data types in question (particularly numeric types like float and int) in
 * default C format, you'll need to switch back to the C locale before the call.
 * This problem does not exist with the newer from_string function template.
 */
template<typename T> inline void FromString(const char Str[], T &Obj)
{
  if (!Str) throw PGSTD::runtime_error("Attempt to convert NULL string to " +
		                     PGSTD::string(typeid(T).name()));

  if (sscanf(Str, internal::FmtString(Obj), &Obj) != 1)
    throw PGSTD::runtime_error("Cannot convert value '" + 
		             PGSTD::string(Str) + 
			     "' to " + typeid(T).name());
}


namespace internal
{
/// For libpqxx internal use only: convert C string to C++ string
/** @deprecated To be removed when FromString is taken out
 */
void PQXX_LIBEXPORT FromString_string(const char Str[], PGSTD::string &Obj);

/// For libpqxx internal use only: convert unsigned char * to C++ string
/** @deprecated To be removed when FromString is taken out
 */
void PQXX_LIBEXPORT FromString_ucharptr(const char Str[], 
    	const unsigned char *&Obj);

/// For libpqxx internal use only: quote std::string
PGSTD::string PQXX_LIBEXPORT Quote_string(const PGSTD::string &Obj, 
    	bool EmptyIsNull);

/// For libpqxx internal use only: quote const char *
PGSTD::string PQXX_LIBEXPORT Quote_charptr(const char Obj[], bool EmptyIsNull);
} // namespace internal


template<> inline void FromString(const char Str[], PGSTD::string &Obj)
{
  internal::FromString_string(Str, Obj);
}

template<> inline void FromString(const char Str[], const char *&Obj)
{
  if (!Str) throw PGSTD::runtime_error("Attempt to read NULL string");
  Obj = Str;
}

template<> inline void FromString(const char Str[], const unsigned char *&Obj)
{
  internal::FromString_ucharptr(Str, Obj);
}

template<> inline void FromString(const char Str[], bool &Obj)
{
  from_string(Str, Obj);
}


/// Escape nul-terminated string for inclusion in SQL strings
/** Use this to sanitize strings that may contain characters like backslashes
 * or quotes.  You'll want to do this for all data received from outside your
 * application that gets used in SQL--otherwise an attacker might crack your
 * code by feeding it some string containing e.g. a closing quote followed by
 * SQL commands you did not intend to execute.
 *
 * Unlike its predecessor Quote(), this function does not add SQL-style single
 * quotes around the result string; nor does it recognize and generate nulls.
 */
PGSTD::string sqlesc(const char str[]);					//[t0]

/// Escape string for inclusion in SQL strings
/** Reads and escapes input string.  The string is terminated by either a nul
 * character or the given byte length, whichever comes first.
 *
 * @param str optionally zero-terminated character string
 * @param maxlen largest possible length of input string, not including optional
 * terminating nul character.
 *
 * Unlike its predecessor Quote(), this function does not add SQL-style single
 * quotes around the result string; nor does it recognize and generate nulls.
 */
PGSTD::string sqlesc(const char str[], size_t maxlen);			//[t0]

/// Escape string for inclusion in SQL strings
/** This function differs from similar ones based on libpq in that it handles
 * embedded nul bytes correctly.
 *
 * Unlike its predecessor Quote(), this function does not add SQL-style single
 * quotes around the result string; nor does it recognize and generate nulls.
 */
PGSTD::string sqlesc(const PGSTD::string &);				//[t0]


/// Quote string for use in SQL
/** Generate SQL-quoted version of string.  If EmptyIsNull is set, an empty
 * string will generate the null value rather than an empty string.
 * @deprecated Use sqlesc instead.
 */
template<typename T> PGSTD::string Quote(const T &Obj, bool EmptyIsNull);


/// std::string version, on which the other versions are built
/** @deprecated Use sqlesc instead.
 */
template<> 
inline PGSTD::string Quote(const PGSTD::string &Obj, bool EmptyIsNull)
{
  return internal::Quote_string(Obj, EmptyIsNull);
}

/// Special case for const char *, accepting null pointer as null value
/** @deprecated Use sqlesc instead.
 */
template<> inline PGSTD::string Quote(const char *const & Obj, bool EmptyIsNull)
{
  return internal::Quote_charptr(Obj, EmptyIsNull);
}


/// Specialization for string constants
/** This specialization is a little complicated, because string constants are
 * of the type char[], not of type const char * as one might expect.  Note that
 * the size of the array is part of the type, for which reason we need it in our
 * template here.
 */
template<int LEN> inline PGSTD::string Quote(const char (&Obj)[LEN],
    					     bool EmptyIsNull)
{
  return internal::Quote_charptr(Obj, EmptyIsNull);
}


template<typename T> inline PGSTD::string Quote(const T &Obj, bool EmptyIsNull)
{
  return Quote(ToString(Obj), EmptyIsNull);
}


/// Quote string for use in SQL
/** This version of the function never generates null values.
 * @deprecated Use sqlesc instead.
 */
template<typename T> inline PGSTD::string Quote(T Obj)
{
  return Quote(Obj, false);
}


namespace internal
{
void freepqmem(void *);
void freenotif(PQXXPQ::PGnotify *);

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
  typedef T content_type;

  PQAlloc() : m_Obj(0) {}

  /// Assume ownership of a pointer
  explicit PQAlloc(T *obj) : m_Obj(obj) {}

  ~PQAlloc() throw () { close(); }

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
    freepqmem(m_Obj);
  }

  PQAlloc(const PQAlloc &);		// Not allowed
  PQAlloc &operator=(const PQAlloc &);	// Not allowed
};


/// Special version for notify structures, using PQfreeNotify() if available
template<> inline void PQAlloc<PQXXPQ::PGnotify>::freemem() throw ()
{
  freenotif(m_Obj);
}


class PQXX_LIBEXPORT namedclass
{
public:
  namedclass(const PGSTD::string &Name, const PGSTD::string &Classname) :
    m_Name(Name),
    m_Classname(Classname)
  {
  }

  const PGSTD::string &name() const throw () { return m_Name; }		//[t1]
  const PGSTD::string &classname() const throw () {return m_Classname;}	//[t73]
  PGSTD::string description() const;

private:
  PGSTD::string m_Name, m_Classname;
};


void CheckUniqueRegistration(const namedclass *New, const namedclass *Old);
void CheckUniqueUnregistration(const namedclass *New, const namedclass *Old);


/// Ensure proper opening/closing of GUEST objects related to a "host" object
/** Only a single GUEST may exist for a single host at any given time.  GUEST
 * must be derived from namedclass.
 */
template<typename GUEST>
class unique
{
public:
  unique() : m_Guest(0) {}

  GUEST *get() const throw () { return m_Guest; }

  void Register(GUEST *G)
  {
    CheckUniqueRegistration(G, m_Guest);
    m_Guest = G;
  }

  void Unregister(GUEST *G)
  {
    CheckUniqueUnregistration(G, m_Guest);
    m_Guest = 0;
  }

private:
  GUEST *m_Guest;

  /// Not allowed
  unique(const unique &);
  /// Not allowed
  unique &operator=(const unique &);
};

/// Sleep for the given number of seconds
void sleep_seconds(int);

} // namespace internal
} // namespace pqxx

