/* String conversion definitions.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stringconv instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_STRINGCONV
#define PQXX_H_STRINGCONV

#include "pqxx/compiler-public.hxx"

#include <limits>
#include <optional>
#include <sstream>
#include <stdexcept>


namespace pqxx::internal
{
/// Throw exception for attempt to convert null to given type.
[[noreturn]] PQXX_LIBEXPORT void throw_null_conversion(
	const std::string &type);


/// Helper: string traits implementation for built-in types.
/** These types all look much alike, so they can share much of their traits
 * classes (though templatised, of course).
 *
 * The actual `to_string` and `from_string` are implemented in the library,
 * but the rest is defined inline.
 */
template<typename TYPE> struct PQXX_LIBEXPORT builtin_traits
{
  static constexpr bool has_null() noexcept { return false; }
  static constexpr bool is_null(TYPE) { return false; }
  static void from_string(std::string_view str, TYPE &obj);
  static std::string to_string(TYPE Obj);
};


/// Compute numeric value of given textual digit (assuming that it is a digit)
constexpr int digit_to_number(char c) noexcept { return c-'0'; }
constexpr char number_to_digit(int i) noexcept
	{ return static_cast<char>(i+'0'); }
} // namespace pqxx::internal


namespace pqxx
{
/**
 * @defgroup stringconversion String conversion
 *
 * The PostgreSQL server accepts and represents data in string form.  It has
 * its own formats for various data types.  The string conversions define how
 * various C++ types translate to and from their respective PostgreSQL text
 * representations.
 *
 * Each conversion is defined by a specialisation of the @c string_traits
 * template.  This template implements some basic functions to support the
 * conversion, ideally in both directions.
 *
 * If you need to convert a type which is not supported out of the box, define
 * your own @c string_traits specialisation for that type, similar to the ones
 * defined here.  Any conversion code which "sees" your specialisation will now
 * support your conversion.  In particular, you'll be able to read result
 * fields into a variable of the new type.
 *
 * There is a macro to help you define conversions for individual enumeration
 * types.  The conversion will represent enumeration values as numeric strings.
 */
//@{

/// A human-readable name for a type, used in error messages and such.
template<typename TYPE> const std::string type_name;


template<typename TYPE> const std::string type_name<std::optional<TYPE>> =
	"opt<" + type_name<TYPE> + ">";

/// Define a @c type_name for TYPE.  Use inside the @c pqxx namespace.
#define PQXX_DECLARE_TYPE_NAME(TYPE) \
  template<> const std::string type_name<TYPE> = #TYPE


/// Traits class for use in string conversions
/** Specialize this template for a type that you wish to add to_string and
 * from_string support for.
 */
template<typename T, typename = void> struct string_traits;


/// Helper class for defining enum conversions.
/** The conversion will convert enum values to numeric strings, and vice versa.
 *
 * To define a string conversion for an enum type, derive a @c string_traits
 * specialisation for the enum from this struct.
 *
 * There's usually an easier way though: the @c PQXX_DECLARE_ENUM_CONVERSION
 * macro.  Use @c enum_traits manually only if you need to customise your
 * traits type in more detail, e.g. if your enum has a "null" value built in.
 */
template<typename ENUM>
struct enum_traits
{
  using underlying_type = typename std::underlying_type<ENUM>::type;
  using underlying_traits = string_traits<underlying_type>;

  static constexpr bool has_null() noexcept { return false; }
  [[noreturn]] static ENUM null()
	{ internal::throw_null_conversion("enum type"); }

  static void from_string(std::string_view str, ENUM &obj)
  {
    underlying_type tmp;
    underlying_traits::from_string(str, tmp);
    obj = ENUM(tmp);
  }

  static std::string to_string(ENUM Obj)
	{ return underlying_traits::to_string(underlying_type(Obj)); }
};


/// Macro: Define a string conversion for an enum type.
/** This specialises the @c pqxx::string_traits template, so use it in the
 * @c ::pqxx namespace.
 *
 * For example:
 *
 *      #include <iostream>
 *      #include <pqxx/strconv>
 *      enum X { xa, xb };
 *      namespace pqxx { PQXX_DECLARE_ENUM_CONVERSION(x); }
 *      int main() { std::cout << to_string(xa) << std::endl; }
 */
#define PQXX_DECLARE_ENUM_CONVERSION(ENUM) \
PQXX_DECLARE_TYPE_NAME(ENUM); \
template<> \
struct string_traits<ENUM> : pqxx::enum_traits<ENUM> \
{ \
  [[noreturn]] static ENUM null() \
	{ internal::throw_null_conversion(type_name<ENUM>); } \
}

//@}
} // namespace pqxx


namespace pqxx
{
PQXX_DECLARE_TYPE_NAME(bool);
PQXX_DECLARE_TYPE_NAME(short);
PQXX_DECLARE_TYPE_NAME(unsigned short);
PQXX_DECLARE_TYPE_NAME(int);
PQXX_DECLARE_TYPE_NAME(unsigned int);
PQXX_DECLARE_TYPE_NAME(long);
PQXX_DECLARE_TYPE_NAME(unsigned long);
PQXX_DECLARE_TYPE_NAME(long long);
PQXX_DECLARE_TYPE_NAME(unsigned long long);
PQXX_DECLARE_TYPE_NAME(float);
PQXX_DECLARE_TYPE_NAME(double);
PQXX_DECLARE_TYPE_NAME(long double);
PQXX_DECLARE_TYPE_NAME(char *);
PQXX_DECLARE_TYPE_NAME(const char *);
PQXX_DECLARE_TYPE_NAME(std::string);
PQXX_DECLARE_TYPE_NAME(const std::string);
PQXX_DECLARE_TYPE_NAME(std::stringstream);
PQXX_DECLARE_TYPE_NAME(std::nullptr_t);

template<size_t N> const std::string type_name<char[N]> = "char[]";

/// Helper: declare a string_traits specialisation for a builtin type.
#define PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(TYPE) \
  template<> struct PQXX_LIBEXPORT string_traits<TYPE> : \
    internal::builtin_traits<TYPE> \
    { \
      [[noreturn]] static TYPE null() \
	{ pqxx::internal::throw_null_conversion(type_name<TYPE>); } \
    }

PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(bool);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(short);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(unsigned short);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(int);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(unsigned int);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(long);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(unsigned long);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(long long);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(unsigned long long);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(float);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(double);
PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION(long double);
#undef PQXX_DECLARE_STRING_TRAITS_SPECIALIZATION


/// String traits for C-style string ("pointer to const char")
template<> struct PQXX_LIBEXPORT string_traits<const char *>
{
  static constexpr bool has_null() noexcept { return true; }
  static constexpr bool is_null(const char *t) { return t == nullptr; }
  static constexpr const char *null() { return nullptr; }
  static void from_string(std::string_view str, const char *&obj)
	{ obj = str.data(); }
  static std::string to_string(const char *obj) { return obj; }
};

/// String traits for non-const C-style string ("pointer to char")
template<> struct PQXX_LIBEXPORT string_traits<char *>
{
  static constexpr bool has_null() noexcept { return true; }
  static constexpr bool is_null(const char *t) { return t == nullptr; }
  static constexpr const char *null() { return nullptr; }

  // Don't allow this conversion since it breaks const-safety.
  // static void from_string(std::string_view str, char *&obj);

  static std::string to_string(char *obj) { return obj; }
};

/// String traits for C-style string constant ("array of char")
template<size_t N> struct PQXX_LIBEXPORT string_traits<char[N]>
{
  static constexpr bool has_null() noexcept { return true; }
  static constexpr bool is_null(const char t[]) { return t == nullptr; }
  static constexpr const char *null() { return nullptr; }
  static std::string to_string(const char Obj[]) { return Obj; }
};

template<> struct PQXX_LIBEXPORT string_traits<std::string>
{
  static constexpr bool has_null() noexcept { return false; }
  static constexpr bool is_null(const std::string &) { return false; }
  [[noreturn]] static std::string null()
	{ internal::throw_null_conversion(type_name<std::string>); }
  static void from_string(std::string_view str, std::string &obj)
	{ obj = str; }
  static std::string to_string(const std::string &Obj) { return Obj; }
};

template<> struct PQXX_LIBEXPORT string_traits<const std::string>
{
  static constexpr bool has_null() noexcept { return false; }
  static constexpr bool is_null(const std::string &) { return false; }
  [[noreturn]] static const std::string null()
	{ internal::throw_null_conversion(type_name<std::string>); }
  static std::string to_string(const std::string &Obj) { return Obj; }
};

template<> struct PQXX_LIBEXPORT string_traits<std::stringstream>
{
  static constexpr bool has_null() noexcept { return false; }
  static constexpr bool is_null(const std::stringstream &) { return false; }
  [[noreturn]] static std::stringstream null()
	{ internal::throw_null_conversion(type_name<std::stringstream>); }
  static void from_string(std::string_view str, std::stringstream &obj)
	{ obj.clear(); obj << str; }
  static std::string to_string(const std::stringstream &Obj)
	{ return Obj.str(); }
};

/// Weird case: nullptr_t.  We don't fully support it.
template<> struct PQXX_LIBEXPORT string_traits<std::nullptr_t>
{
  static constexpr bool has_null() noexcept { return true; }
  static constexpr bool is_null(std::nullptr_t) noexcept { return true; }
  static constexpr std::nullptr_t null() { return nullptr; }
  static std::string to_string(const std::nullptr_t &)
	{ return "null"; }
};


// TODO: Implement date conversions.

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
template<typename T> inline void from_string(std::string_view str, T &obj)
{
  if (str.data() == nullptr)
    throw std::runtime_error{"Attempt to read null string."};
  string_traits<T>::from_string(str, obj);
}


template<typename T>
inline void from_string(const std::stringstream &str, T &obj)		//[t00]
	{ from_string(str.str(), obj); }


/// Convert built-in type to a readable string that PostgreSQL will understand
/** No special formatting is done, and any locale settings are ignored.  The
 * resulting string will be human-readable and in a format suitable for use in
 * SQL queries.
 */
template<typename T> std::string to_string(const T &Obj)
	{ return string_traits<T>::to_string(Obj); }

} // namespace pqxx
#endif
