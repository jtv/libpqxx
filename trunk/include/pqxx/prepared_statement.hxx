/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/prepared_statement.hxx
 *
 *   DESCRIPTION
 *      Helper classes for defining and executing prepared statements
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
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
class PQXX_LIBEXPORT param_declaration
{
public:
  param_declaration(connection_base &, const PGSTD::string &statement);

  /// Add a parameter specification to prepared statement declaration
  const param_declaration &operator()(const PGSTD::string &sqltype,
	param_treatment) const;

private:
  connection_base &m_home;
  const PGSTD::string &m_statement;
};


/// Helper class for passing parameters to, and executing, prepared statements
class PQXX_LIBEXPORT param_value
{
public:
  param_value(transaction_base &, const PGSTD::string &statement);

  result exec();

  /// Pass null parameter
  param_value &operator()();

  /// Pass parameter value
  /**
   * @param v parameter value (will be represented as a string internally)
   * @param nonnull replaces value with null if set to false
   */
  template<typename T>
    param_value &operator()(const T &v, bool nonnull=true)
	{ return setparam(to_string(v), nonnull); }

  /// Pass pointer parameter value, or null if pointer is null
  /**
   * @warning Be very careful with the special constant NULL!  Since NULL in
   * C++ is an int, not a pointer, a value of NULL would cause the wrong
   * version of this template to be invoked.  To all intents and purposes it
   * would look like you were trying to pass a regular zero as an integer
   * value, instead of a null string.  This is not a problem with pointer
   * variables that may happen to be NULL, since in that case the value's type
   * is not subject to any confusion.  If you know at compile time that you
   * want to pass a null value, use the zero-argument version of this
   * operator; if you don't want to do that, at least add a second argument of
   * "false" to make clear that you want a null, not a zero.
   *
   * @param v parameter value (will be represented as a string internally)
   * @param nonnull replaces value with null if set to false
   */
  template<typename T>
    param_value &operator()(T *v, bool nonnull=true)
	{ return setparam((v ? to_string(v) : ""),nonnull); }

private:
  transaction_base &m_home;
  const PGSTD::string &m_statement;
  PGSTD::vector<PGSTD::string> m_values;
  PGSTD::vector<const char *> m_ptrs;

  param_value &setparam(const PGSTD::string &, bool nonnull);
};


namespace internal
{
/// Internal representation of a prepared statement definition
struct PQXX_PRIVATE prepared_def
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

} // namespace pqxx::prepared::internal
} // namespace pqxx::prepared
} // namespace pqxx

