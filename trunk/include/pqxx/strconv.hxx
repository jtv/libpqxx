/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/stringconv.hxx
 *
 *   DESCRIPTION
 *      String conversion definitions for libpqxx
 *      DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stringconv instead.
 *
 * Copyright (c) 2008-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_STRINGCONV
#define PQXX_H_STRINGCONV

#include "pqxx/compiler-public.hxx"

#include <sstream>
#include <stdexcept>


namespace pqxx
{

/**
 * @defgroup stringconversion String conversion
 *
 * For purposes of communication with the server, values need to be converted
 * from and to a human-readable string format that (unlike the various functions
 * and templates in the C and C++ standard libraries) is not sensitive to locale
 * settings and internationalization.  This section contains functionality that
 * is used extensively by libpqxx itself, but is also available for use by other
 * programs.
 */
//@{

/// Traits class for use in string conversions
/** Specialize this template for a type that you wish to add to_string and
 * from_string support for.
 */
template<typename T> struct string_traits {};

namespace internal
{
/// Throw exception for attempt to convert null to given type.
PQXX_NORETURN PQXX_LIBEXPORT void throw_null_conversion(
	const std::string &type);
} // namespace pqxx::internal

#define PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(T)			\
template<> struct PQXX_LIBEXPORT string_traits<T>			\
{									\
  typedef T subject_type;						\
  static const char *name() { return #T; }				\
  static bool has_null() { return false; }				\
  static bool is_null(T) { return false; }				\
  static T null() 							\
    { internal::throw_null_conversion(name()); return subject_type(); }	\
  static void from_string(const char Str[], T &Obj);			\
  static std::string to_string(T Obj);					\
};

PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(bool)

PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(short)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(unsigned short)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(int)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(unsigned int)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(long)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(unsigned long)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(long long)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(unsigned long long)

PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(float)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(double)
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(long double)

#undef PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION

/// String traits for C-style string ("pointer to const char")
template<> struct PQXX_LIBEXPORT string_traits<const char *>
{
  static const char *name() { return "const char *"; }
  static bool has_null() { return true; }
  static bool is_null(const char *t) { return !t; }
  static const char *null() { return NULL; }
  static void from_string(const char Str[], const char *&Obj) { Obj = Str; }
  static std::string to_string(const char *Obj) { return Obj; }
};

/// String traits for non-const C-style string ("pointer to char")
template<> struct PQXX_LIBEXPORT string_traits<char *>
{
  static const char *name() { return "char *"; }
  static bool has_null() { return true; }
  static bool is_null(const char *t) { return !t; }
  static const char *null() { return NULL; }

  // Don't allow this conversion since it breaks const-safety.
  // static void from_string(const char Str[], char *&Obj);

  static std::string to_string(char *Obj) { return Obj; }
};

/// String traits for C-style string constant ("array of char")
template<size_t N> struct PQXX_LIBEXPORT string_traits<char[N]>
{
  static const char *name() { return "char[]"; }
  static bool has_null() { return true; }
  static bool is_null(const char t[]) { return !t; }
  static const char *null() { return NULL; }
  static std::string to_string(const char Obj[]) { return Obj; }
};

/// String traits for "array of const char."
/** Visual Studio 2010 isn't happy without this redundant specialization.
 * Other compilers shouldn't need it.
 */
template<size_t N> struct PQXX_LIBEXPORT string_traits<const char[N]>
{
  static const char *name() { return "char[]"; }
  static bool has_null() { return true; }
  static bool is_null(const char t[]) { return !t; }
  static const char *null() { return NULL; }
  static std::string to_string(const char Obj[]) { return Obj; }
};


template<> struct PQXX_LIBEXPORT string_traits<std::string>
{
  static const char *name() { return "string"; }
  static bool has_null() { return false; }
  static bool is_null(const std::string &) { return false; }
  static std::string null()
	{ internal::throw_null_conversion(name()); return std::string(); }
  static void from_string(const char Str[], std::string &Obj) { Obj=Str; }
  static std::string to_string(const std::string &Obj) { return Obj; }
};

template<> struct PQXX_LIBEXPORT string_traits<const std::string>
{
  static const char *name() { return "const string"; }
  static bool has_null() { return false; }
  static bool is_null(const std::string &) { return false; }
  static const std::string null()
	{ internal::throw_null_conversion(name()); return std::string(); }
  static const std::string to_string(const std::string &Obj) { return Obj; }
};

template<> struct PQXX_LIBEXPORT string_traits<std::stringstream>
{
  static const char *name() { return "stringstream"; }
  static bool has_null() { return false; }
  static bool is_null(const std::stringstream &) { return false; }
  static std::stringstream null()
  {
    internal::throw_null_conversion(name());
    // No, dear compiler, we don't need a return here.
    throw 0;
  }
  static void from_string(const char Str[], std::stringstream &Obj)
                                                    { Obj.clear(); Obj << Str; }
  static std::string to_string(const std::stringstream &Obj)
                                                           { return Obj.str(); }
};


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
template<typename T>
  inline void from_string(const char Str[], T &Obj)
{
  if (!Str)
    throw std::runtime_error("Attempt to read NULL string");
  string_traits<T>::from_string(Str, Obj);
}


/// Conversion with known string length (for strings that may contain nuls)
/** This is only used for strings, where embedded nul bytes should not determine
 * the end of the string.
 *
 * For all other types, this just uses the regular, nul-terminated version of
 * from_string().
 */
template<typename T> inline void from_string(const char Str[], T &Obj, size_t)
{
  return from_string(Str, Obj);
}

template<>
  inline void from_string<std::string>(const char Str[],
	std::string &Obj,
	size_t len)							//[t0]
{
  if (!Str)
    throw std::runtime_error("Attempt to read NULL string");
  Obj.assign(Str, len);
}

template<typename T>
  inline void from_string(const std::string &Str, T &Obj)		//[t45]
	{ from_string(Str.c_str(), Obj); }

template<typename T>
  inline void from_string(const std::stringstream &Str, T &Obj)		//[t0]
	{ from_string(Str.str(), Obj); }

template<> inline void
from_string(const std::string &Str, std::string &Obj)			//[t46]
	{ Obj = Str; }


namespace internal
{
/// Compute numeric value of given textual digit (assuming that it is a digit)
inline int digit_to_number(char c) PQXX_NOEXCEPT { return c-'0'; }
inline char number_to_digit(int i) PQXX_NOEXCEPT
	{ return static_cast<char>(i+'0'); }
} // namespace pqxx::internal


/// Convert built-in type to a readable string that PostgreSQL will understand
/** No special formatting is done, and any locale settings are ignored.  The
 * resulting string will be human-readable and in a format suitable for use in
 * SQL queries.
 */
template<typename T> inline std::string to_string(const T &Obj)
	{ return string_traits<T>::to_string(Obj); }

//@}

} // namespace pqxx

#endif

