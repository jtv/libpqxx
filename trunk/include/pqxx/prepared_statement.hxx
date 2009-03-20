/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/prepared_statement.hxx
 *
 *   DESCRIPTION
 *      Helper classes for defining and executing prepared statements
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006-2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_PREPARED_STATEMENT
#define PQXX_H_PREPARED_STATEMENT

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/strconv"
#include "pqxx/util"


namespace pqxx
{
class connection_base;
class transaction_base;
class result;

/// Dedicated namespace for helper types related to prepared statements
namespace prepare
{
/// Type of treatment of a particular parameter to a prepared statement
/** This information is needed to determine whether a parameter needs to be
 * quoted, escaped, binary-escaped, and/or converted to boolean as it is
 * passed to a prepared statement on execution.
 *
 * This treatment becomes relevant when either the available libpq version
 * doesn't provide direct support for prepared statements, so the definition
 * must be generated as SQL.  This is the case with libpq versions prior to the
 * one shipped with PostgreSQL 7.4).
 */
enum param_treatment
{
  /// Pass as raw, binary bytes
  treat_binary,
  /// Escape special characters and add quotes
  treat_string,
  /// Represent as named Boolean value
  treat_bool,
  /// Include directly in SQL without conversion (e.g. for numeric types)
  treat_direct
};


/// Helper class for declaring parameters to prepared statements
/** You probably won't want to use this class.  It's here just so you can
 * declare parameters by adding parenthesized declarations directly after the
 * statement declaration itself:
 *
 * @code
 * C.prepare(name, query)(paramtype1)(paramtype2, treatment)(paramtype3);
 * @endcode
 */
class PQXX_LIBEXPORT declaration
{
public:
  declaration(connection_base &, const PGSTD::string &statement);

  /// Add a parameter specification to prepared statement declaration
  const declaration &
  operator()(const PGSTD::string &sqltype, param_treatment=treat_direct) const;

  /// Permit arbitrary parameters after the last declared one.
  /**
   * When used, this allows an arbitrary number of parameters to be passed after
   * the last declared one.  This is similar to the C language's varargs.
   *
   * Calling this completes the declaration; no parameters can be declared after
   * etc().
   */
  const declaration &etc(param_treatment=treat_direct) const;

private:
  /// Not allowed
  declaration &operator=(const declaration &);

  connection_base &m_home;
  const PGSTD::string m_statement;
};


/// Helper class for passing parameters to, and executing, prepared statements
class PQXX_LIBEXPORT invocation
{
public:
  invocation(transaction_base &, const PGSTD::string &statement);

  /// Execute!
  result exec() const;

  /// Pass null parameter
  invocation &operator()();

  /// Pass parameter value
  /**
   * @param v parameter value (will be represented as a string internally)
   */
  template<typename T>
    invocation &operator()(const T &v)
  {
    const bool nonnull = !pqxx::string_traits<T>::is_null(v);
    return setparam((nonnull ? pqxx::to_string(v) : ""), nonnull);
  }

  /// Pass parameter value
  /**
   * @param v parameter value (will be represented as a string internally)
   * @param nonnull replaces value with null if set to false
   */
  template<typename T>
    invocation &operator()(const T &v, bool nonnull)
	{ return setparam((nonnull ? pqxx::to_string(v) : ""), nonnull); }

  /// Pass C-style parameter string, or null if pointer is null
  /**
   * This version is for passing C-style strings; it's a template, so any
   * pointer type that @c to_string accepts will do.
   *
   * @warning Be very careful with the special constant @c NULL!  Since @c NULL
   * in C++ is an @c int, not a pointer, a value of @c NULL would cause the
   * wrong version of this template to be invoked.  To all intents and purposes
   * it would look like you were trying to pass a regular zero as an integer
   * value, instead of a null string.  This is not a problem with pointer
   * variables that may happen to be @c NULL, since in that case the value's
   * type is not subject to any confusion.  So if you know at compile time that
   * you want to pass a null value, use the zero-argument version of this
   * operator; if you don't want to do that, at least add a second argument of
   * @c false to make clear that you want a null, not a zero.
   *
   * @param v parameter value (will be represented as a C++ string internally)
   * @param nonnull replaces value with null if set to @c false
   */
  template<typename T>
    invocation &operator()(T *v, bool nonnull=true)
	{ return setparam((v ? to_string(v) : ""), nonnull); }

  /// Pass C-style string parameter, or null if pointer is null
  /** This duplicates the pointer-to-template-argument-type version of the
   * operator, but helps compilers with less advanced template implementations
   * disambiguate calls where C-style strings are passed.
   */
  invocation &operator()(const char *v, bool nonnull=true)
	{ return setparam((v ? to_string(v) : ""), nonnull); }

private:
  /// Not allowed
  invocation &operator=(const invocation &);

  transaction_base &m_home;
  const PGSTD::string m_statement;
  PGSTD::vector<PGSTD::string> m_values;
  PGSTD::vector<bool> m_nonnull;

  invocation &setparam(const PGSTD::string &, bool nonnull);
};


namespace internal
{
/// Internal representation of a prepared statement definition
struct PQXX_LIBEXPORT prepared_def
{
  /// Parameter definition
  struct param
  {
    PGSTD::string sqltype;
    param_treatment treatment;

    param(const PGSTD::string &SQLtype, param_treatment);
  };

  /// Text of prepared query
  PGSTD::string definition;
  /// Parameter list
  PGSTD::vector<param> parameters;
  /// Has this prepared statement been prepared in the current session?
  bool registered;
  /// Is this definition complete?
  bool complete;

  /// Does this statement accept variable arguments, as declared with etc()?
  bool varargs;

  /// How should parameters after the last declared one be treated?
  param_treatment varargs_treatment;

  prepared_def();
  explicit prepared_def(const PGSTD::string &);

  void addparam(const PGSTD::string &sqltype, param_treatment);
};

/// Utility functor: get prepared-statement parameter's SQL type string
struct PQXX_PRIVATE get_sqltype
{
  template<typename IT> const PGSTD::string &operator()(IT i)
  {
    return i->sqltype;
  }
};

} // namespace pqxx::prepare::internal
} // namespace pqxx::prepare
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"

#endif

