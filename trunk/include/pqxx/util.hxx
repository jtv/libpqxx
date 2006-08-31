/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/util.hxx
 *
 *   DESCRIPTION
 *      Various utility definitions for libpqxx
 *      DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/util instead.
 *
 * Copyright (c) 2001-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-public.hxx"

#include <cstdio>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

/** @mainpage
 * @author Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * Welcome to libpqxx, the C++ API to the PostgreSQL database management system.
 *
 * Compiling this package requires PostgreSQL to be installed--including the C
 * headers for client development.  The library builds on top of PostgreSQL's
 * standard C API, libpq.  The libpq headers are not needed to compile client
 * programs, however.
 *
 * For a quick introduction to installing and using libpqxx, see the README
 * file; a more extensive tutorial is available in doc/html/Tutorial/index.html.
 *
 * The latest information, as well as updates, a mailing list, and a bug
 * reporting system can be found at the project's home page.
 *
 * @see http://thaiopensource.org/development/libpqxx/
 */

/** @page threading Thread safety
 * 
 * This library does not contain any locking code to protect objects against
 * simultaneous modification in multi-threaded programs.  Therefore it is up to
 * you, the user of the library, to ensure that your client programs perform no
 * conflicting operations simultaneously in multiple threads.
 *
 * The reason for this is simple: there are many different threading interfaces
 * and libpqxx does not mean to impose a choice.  Additionally, locking incurs a
 * performance overhead without benefitting most programs.
 *
 * It's not all bad news, however.  The library tries to avoid unsafe operations
 * and so does the underlying libpq.  Apart from a few exceptions--which should
 * generally be noted in this documentation--all your program needs to do to
 * maintain thread safety is to ensure that no two threads perform a non-const
 * operation on a single object simultaneously.
 */

/// The home of all libpqxx classes, functions, templates, etc.
namespace pqxx {}

/// Alias for the standard namespace "<tt>std</tt>"
/** This alias is used throughout libpqxx to accomodate the use of other
 * standard library implementations than the one native to the compiler.  These
 * alternative implementations may live in separate namespaces to avoid
 * confusion.
 */
namespace PGSTD {}

#include <pqxx/libpq-forward.hxx>


namespace pqxx
{
/// The "null" oid
const oid oid_none = 0;

/**
 * @defgroup stringconversion String conversion
 *
 * For purposes of communication with the server, values need to be converted
 * from and to a human-readable string format that (unlike the various functions
 * and templates in the C and C++ standard libraries) is not sensitive to locale
 * settings and internationalization.  This section contains functionality that
 * is used extensively by libpqxx itself, but is also available for use by other
 * programs.
 *
 * Some conversions are considered to be ambiguous.  An example is the
 * conversion between @c char and string: is the @c char intended as a character
 * (in which case there are easier ways to accomplish the conversion), or is it
 * being used as merely a very small integral type?  And in the latter case,
 * what range is it expected to have--signed, unsigned, or only the range that
 * those two share?  An ambiguous conversion attempt will result in a build
 * error, typically a linker message complaining about a missing function whose
 * name starts with "<tt>error_</tt>".  Such errors are always deliberately
 * generated.  When you see one, look up the documentation for the function it
 * names.
 */
//@{

/// Dummy name, used by libpqxx in deliberate link errors
/** If you get an error involving this function while building your program,
 * this means that the program contains an unsupported string conversion.
 *
 * In other words, the program tries to convert an object to a string, or a
 * string to an object, of a type for which libpqxx does not implement this
 * conversion.  A notable example is "<tt>long long</tt>," which is supported by
 * many compilers but does not exist in Standard C++.
 *
 * In the case of "<tt>long long</tt>" and similar types, if your implementation
 * of the standard C++ library supports it, you may use a @c stringstream to
 * perform the conversion.  For other types, you may have to write the
 * conversion routine yourself.
 */
template<typename T> void error_unsupported_type_in_string_conversion(T);


/// Dummy name, used to generate meaningful link errors
/** If you get a link error naming this function, this means that your program
 * includes a string conversion involving a signed or unsigned @c char type.
 * The problem with such conversions is that they are ambiguous: do you want to
 * treat the @c char type as a small numeric type, or as a character type like
 * plain @c char?
 */
template<typename T> PGSTD::string error_ambiguous_string_conversion(T);



// TODO: Implement date conversions

/// Attempt to convert postgres-generated string to given built-in type
/** If the form of the value found in the string does not match the expected
 * type, e.g. if a decimal point is found when converting to an integer type,
 * the conversion fails.  Overflows (e.g. converting "9999999999" to a 16-bit
 * C++ type) are also treated as errors.  If in some cases this behaviour should
 * be inappropriate, convert to something bigger such as @c long @c int first
 * and then truncate the resulting value.
 *
 * Only the simplest possible conversions are supported.  No fancy features
 * such as hexadecimal or octal, spurious signs, or exponent notation will work.
 * No whitespace is stripped away.  Only the kinds of strings that come out of
 * PostgreSQL and out of to_string() can be converted.
 */
template<typename T> void from_string(const char Str[], T &Obj);

/// Conversion with known string length (for strings that may contain nuls)
/** This is only used for strings, where embedded nul bytes should not determine
 * the end of the string.
 *
 * For all other types, this just uses the regular, nul-terminated version of
 * from_string().
 */
template<typename T> void from_string(const char Str[], T &Obj, size_t)
{
  return from_string(Str, Obj);
}

template<> void PQXX_LIBEXPORT from_string(const char Str[],
	PGSTD::string &,
	size_t);							//[t0]

template<> void PQXX_LIBEXPORT from_string(const char Str[], long &);	//[t45]
template<>
  void PQXX_LIBEXPORT from_string(const char Str[], unsigned long &);	//[t45]
template<> void PQXX_LIBEXPORT from_string(const char Str[], int &);	//[t45]
template<>
  void PQXX_LIBEXPORT from_string(const char Str[], unsigned int &);	//[t45]
template<> void PQXX_LIBEXPORT from_string(const char Str[], short &);	//[t45]
template<>
  void PQXX_LIBEXPORT from_string(const char Str[], unsigned short &);	//[t45]
template<> void PQXX_LIBEXPORT from_string(const char Str[], float &);	//[t46]
template<> void PQXX_LIBEXPORT from_string(const char Str[], double &);	//[t46]
template<> void PQXX_LIBEXPORT from_string(const char Str[], bool &);	//[t76]
#if defined(PQXX_HAVE_LONG_DOUBLE)
template<>
  void PQXX_LIBEXPORT from_string(const char Str[], long double &);	//[t46]
#endif


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
from_string(const char [], char &Obj)
	{ error_ambiguous_string_conversion(Obj); }
template<> inline void
from_string(const char [], signed char &Obj)
	{ error_ambiguous_string_conversion(Obj); }
template<> inline void
from_string(const char [], unsigned char &Obj)
	{ error_ambiguous_string_conversion(Obj); }

template<> inline void
from_string(const PGSTD::string &, char &Obj)
	{ error_ambiguous_string_conversion(Obj); }
template<> inline void
from_string(const PGSTD::string &, signed char &Obj)
	{ error_ambiguous_string_conversion(Obj); }
template<> inline void
from_string(const PGSTD::string &, unsigned char &Obj)
	{ error_ambiguous_string_conversion(Obj); }


namespace internal
{
/// Compute numeric value of given textual digit (assuming that it is a digit)
inline int digit_to_number(char c) throw () { return c-'0'; }
inline char number_to_digit(int i) throw () { return static_cast<char>(i)+'0'; }
}


/// Convert built-in type to a readable string that PostgreSQL will understand
/** No special formatting is done, and any locale settings are ignored.  The
 * resulting string will be human-readable and in a format suitable for use in
 * SQL queries.
 */
template<typename T> PGSTD::string to_string(const T &);

template<> PGSTD::string PQXX_LIBEXPORT to_string(const short &);	//[t76]
template<>
  PGSTD::string PQXX_LIBEXPORT to_string(const unsigned short &);	//[t76]
template<> PGSTD::string PQXX_LIBEXPORT to_string(const int &);		//[t10]
template<>
  PGSTD::string PQXX_LIBEXPORT to_string(const unsigned int &);		//[t13]
template<> PGSTD::string PQXX_LIBEXPORT to_string(const long &);	//[t18]
template<>
  PGSTD::string PQXX_LIBEXPORT to_string(const unsigned long &);	//[t20]
template<> PGSTD::string PQXX_LIBEXPORT to_string(const float &);	//[t74]
template<> PGSTD::string PQXX_LIBEXPORT to_string(const double &);	//[t74]
template<> PGSTD::string PQXX_LIBEXPORT to_string(const bool &);	//[t76]
#if defined(PQXX_HAVE_LONG_DOUBLE)
template<> PGSTD::string PQXX_LIBEXPORT to_string(const long double &);	//[t74]
#endif

inline PGSTD::string to_string(const char Obj[]) 			//[t14]
	{ return PGSTD::string(Obj); }

inline PGSTD::string to_string(const PGSTD::stringstream &Obj)		//[t0]
	{ return Obj.str(); }

inline PGSTD::string to_string(const PGSTD::string &Obj) {return Obj;}	//[t21]

template<> PGSTD::string PQXX_LIBEXPORT to_string(const char &);	//[t21]


template<> inline PGSTD::string to_string(const signed char &Obj)
	{ return error_ambiguous_string_conversion(Obj); }
template<> inline PGSTD::string to_string(const unsigned char &Obj)
	{ return error_ambiguous_string_conversion(Obj); }
//@}

/// Container of items with easy contents initialization and string rendering
/** Designed as a wrapper around an arbitrary container type, this class lets
 * you easily create a container object and provide its contents in the same
 * line.  Regular addition methods such as push_back() will also still work, but
 * you can now write things like
 * @code
 *  items<int> numbers; numbers(1)(2)(3)(4);
 * @endcode
 *
 * Up to five elements may be specified directly as constructor arguments, e.g.
 * @code
 * items<int> numbers(1,2,3,4);
 * @endcode
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
  explicit items(const T &t) : CONT() { push_back(t); }			//[t0]
  items(const T &t1, const T &t2) : CONT() 				//[t80]
  	{ push_back(t1); push_back(t2); }
  items(const T &t1, const T &t2, const T &t3) : CONT() 		//[t0]
  	{ push_back(t1); push_back(t2); push_back(t3); }
  items(const T &t1, const T &t2, const T &t3, const T &t4) : CONT() 	//[t0]
  	{ push_back(t1); push_back(t2); push_back(t3); push_back(t4); }
  items(const T&t1,const T&t2,const T&t3,const T&t4,const T&t5):CONT() 	//[t0]
  	{push_back(t1);push_back(t2);push_back(t3);push_back(t4);push_back(t5);}
  /// Copy container
  items(const CONT &c) : CONT(c) {}					//[t0]

  /// Add element to items list
  items &operator()(const T &t)						//[t80]
  {
    push_back(t);
    return *this;
  }
};


namespace internal
{
// TODO: Does standard library provide a ready-made version of this?
/// Functor: dereference iterator
template<typename ITER> struct dereference
{
  typename ITER::value_type operator()(ITER i) const { return *i; }
};
template<typename T> struct deref_ptr { T operator()(T *i) const {return *i;} };
}


/// Access iterators using ACCESS functor, returning separator-separated list
/**
 * @param sep separator string (to be placed between items)
 * @param begin beginning of items sequence
 * @param end end of items sequence
 * @param access functor defining how to dereference sequence elements
 */
template<typename ITER, typename ACCESS> inline
PGSTD::string separated_list(const PGSTD::string &sep,			//[t0]
    ITER begin,
    ITER end,
    ACCESS access)
{
  PGSTD::string result;
  if (begin != end)
  {
    result = to_string(access(begin));
    for (++begin; begin != end; ++begin)
    {
      result += sep;
      result += to_string(access(begin));
    }
  }
  return result;
}

/**
 * @defgroup utility Utility functions
 */
//@{

/// Render sequence as a string, using given separator between items
template<typename ITER> inline PGSTD::string
separated_list(const PGSTD::string &sep, ITER begin, ITER end)		//[t8]
	{ return separated_list(sep,begin,end,internal::dereference<ITER>()); }


/// Render array as a string, using given separator between items
template<typename OBJ> inline PGSTD::string
separated_list(const PGSTD::string &sep, OBJ *begin, OBJ *end)		//[t9]
	{ return separated_list(sep,begin,end,internal::deref_ptr<OBJ>()); }


/// Render items in a container as a string, using given separator
template<typename CONTAINER> inline PGSTD::string
separated_list(const PGSTD::string &sep, const CONTAINER &c)		//[t10]
	{ return separated_list(sep, c.begin(), c.end()); }
//@}

/// Private namespace for libpqxx's internal use; do not access
/** This namespace hides definitions internal to libpqxx.  These are not
 * supposed to be used by client programs, and they may change at any time
 * without notice.
 *
 * Conversely, if you find something in this namespace tremendously useful, by
 * all means do lodge a request for its publication.
 *
 * @warning Here be dragons!
 */
namespace internal
{
typedef unsigned long result_size_type;
typedef long result_difference_type;

/// C-style format strings for various built-in types
/** @deprecated To be removed when ToString and FromString are taken out
 *
 * Only allowed for certain types, for which this function is explicitly
 * specialized.  When attempting to use the template for an unsupported type,
 * the generic version will be instantiated.  This will result in a link error
 * for the symbol error_unsupported_type_in_string_conversion(), with a template
 * argument describing the unsupported input type.
 */
template<typename T> inline const char *FmtString(T t) PQXX_DEPRECATED
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
template<> inline const char *FmtString(char)          { return  "%c"; }
template<> inline const char *FmtString(unsigned char) { return  "%c"; }
#if defined(PQXX_HAVE_LONG_DOUBLE)
template<> inline const char *FmtString(long double)   { return "%Lf"; }
#endif

} // namespace internal

/// Convert object of built-in type to string
/** @deprecated Use the newer, rewritten to_string() instead.
 * @warning The conversion is done using the currently active locale, whereas
 * PostgreSQL expects values in the "default" (C) locale.  This means that if
 * you intend to use this function from a locale that renders the data types in
 * question (particularly numeric types like float and int) differently from the
 * C default, you'll need to switch back to the C locale before the call.
 * This problem does not exist with the newer to_string function template.
 */
template<typename T> inline PGSTD::string ToString(const T &Obj) PQXX_DEPRECATED
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
  PQXX_DEPRECATED
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
/// Internal string-escaping function; does not deal with encodings well
/** @deprecated Use transaction's esc() function instead
 */
PGSTD::string escape_string(const char str[], size_t maxlen);

/// For libpqxx internal use only: convert C string to C++ string
/** @deprecated To be removed when FromString is taken out
 */
void PQXX_LIBEXPORT FromString_string(const char Str[], PGSTD::string &Obj)
  PQXX_DEPRECATED;

/// For libpqxx internal use only: convert unsigned char * to C++ string
/** @deprecated To be removed when FromString is taken out
 */
void PQXX_LIBEXPORT FromString_ucharptr(const char Str[],
    	const unsigned char *&Obj)
  PQXX_DEPRECATED;

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


/**
 * @addtogroup escaping String escaping
 *
 * Use these functions to "groom" user-provided strings before using them in
 * your SQL statements.  This reduces the chance of unexpected failures when
 * users type unexpected characters, but more importantly, it helps prevent
 * so-called SQL injection attacks.
 *
 * To understand what SQL injection vulnerabilities are and why they should be
 * prevented, imagine you use the following SQL statement somewhere in your
 * program:
 *
 * @code
 * 	TX.exec("SELECT number,amount "
 * 		"FROM accounts "
 * 		"WHERE allowed_to_see('" + userid + "','" + password + "')");
 * @endcode
 *
 * This shows a logged-in user important information on all accounts he is
 * authorized to view.  The userid and password strings are variables entered by
 * the user himself.
 *
 * Now, if the user is actually an attacker who knows (or can guess) the general
 * shape of this SQL statement, imagine he enters the following password:
 *
 * @code
 * 	'x') OR ('x' = 'x
 * @endcode
 *
 * Does that make sense to you?  Probably not.  But if this is inserted into the
 * SQL string by the C++ code above, the query becomes:
 *
 * @code
 * 	SELECT number,amount
 * 	FROM accounts
 * 	WHERE allowed_to_see('user','x') OR ('x' = 'x')
 * @endcode
 *
 * Is this what you wanted to happen?  Probably not!  The neat allowed_to_see()
 * clause is completely circumvented by the "<tt>OR ('x' = 'x')</tt>" clause,
 * which is always @c true.  Therefore, the attacker will get to see all
 * accounts in the database!
 *
 * To prevent this from happening, use the transaction's esc() function:
 *
 * @code
 * 	TX.exec("SELECT number,amount "
 * 		"FROM accounts "
 * 		"WHERE allowed_to_see('" + TX.esc(userid) + "', "
 * 			"'" + TX.esc(password) + "')");
 * @endcode
 *
 * Now, the quotes embedded in the attacker's string will be neatly escaped so
 * they can't "break out" of the quoted SQL string they were meant to go into:
 *
 * @code
 * 	SELECT number,amount
 * 	FROM accounts
 * 	WHERE allowed_to_see('user', 'x'') OR (''x'' = ''x')
 * @endcode
 *
 * If you look carefully, you'll see that thanks to the added escape characters
 * (a single-quote is escaped in SQL by doubling it) all we get is a very
 * strange-looking password string--but not a change in the SQL statement.
 */
//@{
/// Escape nul-terminated string for inclusion in SQL strings
/** Use this to sanitize strings that may contain characters like backslashes
 * or quotes.  You'll want to do this for all data received from outside your
 * application that gets used in SQL--otherwise an attacker might crack your
 * code by feeding it some string containing e.g. a closing quote followed by
 * SQL commands you did not intend to execute.
 *
 * Unlike its predecessor Quote(), which is now obsolete, this function does not
 * add SQL-style single quotes around the result string; nor does it recognize
 * and generate nulls.
 *
 * @deprecated Use the transaction's esc() function instead
 */
PGSTD::string PQXX_LIBEXPORT sqlesc(const char str[]);

/// Escape string for inclusion in SQL strings
/** Reads and escapes input string.  The string is terminated by either a nul
 * character or the given byte length, whichever comes first.
 *
 * @param str optionally zero-terminated character string
 * @param maxlen largest possible length of input string, not including optional
 * terminating nul character.
 *
 * Unlike its predecessor Quote(), which is now obsolete, this function does not
 * add SQL-style single quotes around the result string; nor does it recognize
 * and generate nulls.
 *
 * @deprecated Use the transaction's esc() function instead
 */
PGSTD::string PQXX_LIBEXPORT sqlesc(const char str[], size_t maxlen);

/// Escape string for inclusion in SQL strings
/** This works like the other sqlesc() variants, which means that if the string
 * contains a nul character, conversion will stop there.  PostgreSQL does not
 * allow nul characters in strings.
 *
 * @deprecated Use the transaction's esc() function instead
 */
PGSTD::string PQXX_LIBEXPORT sqlesc(const PGSTD::string &);


/// Quote string for use in SQL
/** Generate SQL-quoted version of string.  If EmptyIsNull is set, an empty
 * string will generate the null value rather than an empty string.
 *
 * @deprecated Use the transaction's esc() function instead
 */
template<typename T> PGSTD::string Quote(const T &Obj, bool EmptyIsNull)
  PQXX_DEPRECATED;


/// std::string version, on which the other versions are built
/** @deprecated Use the transaction's esc() function instead
 */
template<>
inline PGSTD::string Quote(const PGSTD::string &Obj, bool EmptyIsNull)
  PQXX_DEPRECATED
{
  return internal::Quote_string(Obj, EmptyIsNull);
}

/// Special case for const char *, accepting null pointer as null value
/** @deprecated Use the transaction's esc() function instead
 */
template<> inline PGSTD::string Quote(const char *const & Obj, bool EmptyIsNull)
  PQXX_DEPRECATED
{
  return internal::Quote_charptr(Obj, EmptyIsNull);
}


/// Specialization for string constants
/** This specialization is a little complicated, because string constants are
 * of the type @c char[], not of type <tt>const char *</tt> as one might expect.
 * Note that the size of the array is part of the type, for which reason we need
 * it in our template here.
 *
 * @deprecated Use the transaction's esc() function instead
 */
template<int LEN> inline PGSTD::string Quote(const char (&Obj)[LEN],
    					     bool EmptyIsNull)
  PQXX_DEPRECATED
{
  return internal::Quote_charptr(Obj, EmptyIsNull);
}


template<typename T> inline PGSTD::string Quote(const T &Obj, bool EmptyIsNull)
  PQXX_DEPRECATED
{
  return Quote(ToString(Obj), EmptyIsNull);
}


/// Quote string for use in SQL
/** This version of the function never generates null values.
 *
 * @deprecated Use the transaction's esc() function instead
 */
template<typename T> inline PGSTD::string Quote(T Obj) PQXX_DEPRECATED
{
  return Quote(Obj, false);
}
//@}


namespace internal
{
void PQXX_LIBEXPORT freepqmem(void *);


/// Helper class used in reference counting (doubly-linked circular list)
class PQXX_LIBEXPORT refcount
{
  refcount *volatile m_l, *volatile m_r;

public:
  refcount();
  ~refcount();

  /// Create additional reference based on existing refcount object
  void makeref(refcount &) throw ();

  /// Drop this reference; return whether we were the last reference
  bool loseref() throw ();

private:
  /// Not allowed
  refcount(const refcount &);
  /// Not allowed
  refcount &operator=(const refcount &);
};


/// Reference-counted smart pointer to libpq-allocated object
/** Keep track of a libpq-allocated object, and free it once all references to
 * it have died.
 *
 * If the available PostgreSQL development files supply @c PQfreemem() or
 * @c PQfreeNotify(), this is used to free the memory.  If not, free() is used
 * instead.  This matters on Windows, where memory allocated by a DLL must be
 * freed by the same DLL.
 *
 * @warning Copying, swapping, and destroying PQAlloc objects that refer to the
 * same underlying libpq-allocated block is <em>not thread-safe</em>.  If you
 * wish to pass reference-counted objects around between threads, make sure that
 * each of these operations is protected against concurrency with similar
 * operations on the same object--or other copies of the same object.
 */
template<typename T> class PQAlloc
{
  T *m_Obj;
  mutable refcount m_rc;
public:
  typedef T content_type;

  PQAlloc() throw () : m_Obj(0), m_rc() {}
  PQAlloc(const PQAlloc &rhs) throw () : m_Obj(0), m_rc() { makeref(rhs); }
  ~PQAlloc() throw () { loseref(); }

  PQAlloc &operator=(const PQAlloc &rhs) throw ()
  	{ redoref(rhs); return *this; }

  /// Assume ownership of a pointer
  /** @warning Don't to this more than once for a given object!
   */
  explicit PQAlloc(T *obj) throw () : m_Obj(obj), m_rc() {}

  void swap(PQAlloc &rhs) throw ()
  {
    PQAlloc tmp(*this);
    *this = rhs;
    rhs = tmp;
  }

  PQAlloc &operator=(T *obj) throw () { redoref(obj); return *this; }

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

  void clear() throw () { loseref(); }

private:
  void makeref(T *p) throw () { m_Obj = p; }

  void makeref(const PQAlloc &rhs) throw ()
  {
    m_Obj = rhs.m_Obj;
    m_rc.makeref(rhs.m_rc);
  }

  /// Free and reset current pointer (if any)
  void loseref() throw ()
  {
    if (m_rc.loseref() && m_Obj) freemem();
    m_Obj = 0;
  }

  void redoref(const PQAlloc &rhs) throw ()
	{ if (rhs.m_Obj != m_Obj) { loseref(); makeref(rhs); } }
  void redoref(T *obj) throw ()
	{ if (obj != m_Obj) { loseref(); makeref(obj); } }

  void freemem() throw () { freepqmem(m_Obj); }
};


void PQXX_LIBEXPORT freemem_result(pq::PGresult *) throw ();
template<> inline void PQAlloc<pq::PGresult>::freemem() throw ()
	{ freemem_result(m_Obj); }

void PQXX_LIBEXPORT freemem_notif(pq::PGnotify *) throw ();
template<> inline void PQAlloc<pq::PGnotify>::freemem() throw ()
	{ freemem_notif(m_Obj); }



template<typename T> class scoped_array
{
  T *m_ptr;
public:
  typedef size_t size_type;
  typedef long difference_type;

  scoped_array() : m_ptr(0) {}
  explicit scoped_array(size_type n) : m_ptr(new T[n]) {}
  explicit scoped_array(T *t) : m_ptr(t) {}
  ~scoped_array() { delete [] m_ptr; }

  T *c_ptr() const throw () { return m_ptr; }
  T &operator*() const throw () { return *m_ptr; }
  T &operator[](difference_type i) const throw () { return m_ptr[i]; }

  scoped_array &operator=(T *t) throw ()
  {
    if (t != m_ptr)
    {
      delete [] m_ptr;
      m_ptr = t;
    }
    return *this;
  }

private:
  /// Not allowed
  scoped_array(const scoped_array &);
  scoped_array &operator=(const scoped_array &);
};


class PQXX_LIBEXPORT namedclass
{
public:
  namedclass(const PGSTD::string &Classname, const PGSTD::string &Name="") :
    m_Classname(Classname),
    m_Name(Name)
  {
  }

  const PGSTD::string &name() const throw () { return m_Name; }		//[t1]
  const PGSTD::string &classname() const throw () {return m_Classname;}	//[t73]
  PGSTD::string description() const;

private:
  PGSTD::string m_Classname, m_Name;
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
/** May return early, e.g. when interrupted by a signal.  Completes instantly if
 * a zero or negative sleep time is requested.
 */
void PQXX_LIBEXPORT sleep_seconds(int);

/// Work around problem with library export directives and pointers
typedef const char *cstring;

/// Human-readable description for error code, possibly using given buffer
/** Wrapper for @c strerror() or @c strerror_r(), as available.  The normal case
 * is to copy the string to the provided buffer, but this may not always be the
 * case.  The result is guaranteed to remain usable for as long as the given
 * buffer does.
 * @param err system error code as copied from errno
 * @param buf caller-provided buffer for message to be stored in, if needed
 * @param len usable size (in bytes) of provided buffer
 * @return human-readable error string, which may or may not reside in buf
 */
cstring PQXX_LIBEXPORT strerror_wrapper(int err, char buf[], PGSTD::size_t len)
  throw ();


/// Commonly used SQL commands
extern const char sql_begin_work[], sql_commit_work[], sql_rollback_work[];

} // namespace internal
} // namespace pqxx

