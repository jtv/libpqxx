/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/prepared_statement.hxx
 *
 *   DESCRIPTION
 *      Helper classes for defining and executing prepared statements
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
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

#include "pqxx/internal/statement_parameters.hxx"


namespace pqxx
{
class binarystring;
class connection_base;
class transaction_base;
class result;


/// Dedicated namespace for helper types related to prepared statements
namespace prepare
{
/** \defgroup prepared Prepared statements
 *
 * Prepared statements are SQL queries that you define once and then invoke
 * as many times as you like, typically with varying parameters.  It's basically
 * a function that you can define ad hoc.
 *
 * If you have an SQL statement that you're going to execute many times in
 * quick succession, it may be more efficient to prepare it once and reuse it.
 * This saves the database backend the effort of parsing complex SQL and
 * figuring out an efficient execution plan.  Another nice side effect is that
 * you don't need to worry about escaping parameters.
 *
 * You create a prepared statement by preparing it on the connection, passing an
 * identifier and its SQL text.  The identifier is the name by which the
 * prepared statement will be known; it should consist of letters, digits, and
 * underscores only and start with a letter.  The name is case-sensitive.
 *
 * @code
 * void prepare_my_statement(pqxx::connection_base &c)
 * {
 *   c.prepare("my_statement", "SELECT * FROM Employee WHERE name = 'Xavier'");
 * }
 * @endcode
 *
 * Once you've done this, you'll be able to call @c my_statement from any
 * transaction you execute on the same connection.  Note that this uses a member
 * function called @c "prepared"; the definition used a member function called
 * @c "prepare".
 *
 * @code
 * pqxx::result execute_my_statement(pqxx::transaction_base &t)
 * {
 *   return t.prepared("my_statement").exec();
 * }
 * @endcode
 *
 * Did I mention that you can pass parameters to prepared statements?  You
 * define those along with the statement.  The query text uses $@c 1, @c $2 etc.
 * as placeholders for the parameters in the SQL text.  Since your C++ compiler
 * doesn't know how many parameters you're going to define, the syntax that lets
 * you do this is a bit strange:
 *
 * @code
 * void prepare_find(pqxx::connection_base &c)
 * {
 *   // Prepare a statement called "find" that looks for employees with a given
 *   // name (parameter 1) whose salary exceeds a given number (parameter 2).
 *   const std::string sql =
 *     "SELECT * FROM Employee WHERE name = $1 AND salary > $2";
 * 
 *   c.prepare("find", sql)()();
 * }
 * @endcode
 *
 * It's the @c ()() that declares the two parameters.  If any of the parameters
 * will be in binary form, you will pass treat_binary in the corresponding pair
 * of parentheses.
 *
 * When invoking the prepared statement, you pass parameter values using a
 * similar syntax.
 *
 * @code
 * pqxx::result execute_find(
 *   pqxx::transaction_base &t, std::string name, int min_salary)
 * {
 *   return t.prepared("find")(name)(min_salary).exec();
 * }
 * @endcode
 *
 * @warning There are cases where prepared statements are actually slower than
 * plain SQL.  Sometimes the backend can produce a better execution plan when it
 * knows the parameter values.  For example, say you've got a web application
 * and you're querying for users with status "inactive" who have email addresses
 * in a given domain name X.  If X is a very popular provider, the best way to
 * plan the query may be to list the inactive users first and then filter for
 * the email addresses you're looking for.  But in other cases, it may be much
 * faster to find matching email addresses first and then see which of their
 * owners are "inactive."  A prepared statement must be planned to fit either
 * case, but a direct query can be optimized based on table statistics, partial
 * indexes, etc.
 */

/// Helper class for passing parameters to, and executing, prepared statements
class PQXX_LIBEXPORT invocation : internal::statement_parameters
{
public:
  invocation(transaction_base &, const PGSTD::string &statement);

  /// Execute!
  result exec() const;

  /// Has a statement of this name been defined?
  bool exists() const;

  /// Pass null parameter.
  invocation &operator()() { add_param(); return *this; }

  /// Pass parameter value.
  /**
   * @param v parameter value; will be represented as a string internally.
   */
  template<typename T> invocation &operator()(const T &v)
	{ add_param(v, true); return *this; }

  /// Pass binary parameter value for a BYTEA field.
  /**
   * @param v binary string; will be passed on directly in binary form.
   */
  invocation &operator()(const binarystring &v)
	{ add_binary_param(v, true); return *this; }

  /// Pass parameter value.
  /**
   * @param v parameter value (will be represented as a string internally).
   * @param nonnull replaces value with null if set to false.
   */
  template<typename T> invocation &operator()(const T &v, bool nonnull)
	{ add_param(v, nonnull); return *this; }

  /// Pass binary parameter value for a BYTEA field.
  /**
   * @param v binary string; will be passed on directly in binary form.
   * @param nonnull determines whether to pass a real value, or NULL.
   */
  invocation &operator()(const binarystring &v, bool nonnull)
	{ add_binary_param(v, nonnull); return *this; }

  /// Pass C-style parameter string, or null if pointer is null.
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
  template<typename T> invocation &operator()(T *v, bool nonnull=true)
	{ add_param(v, nonnull); return *this; }

  /// Pass C-style string parameter, or null if pointer is null.
  /** This duplicates the pointer-to-template-argument-type version of the
   * operator, but helps compilers with less advanced template implementations
   * disambiguate calls where C-style strings are passed.
   */
  invocation &operator()(const char *v, bool nonnull=true)
	{ add_param(v, nonnull); return *this; }

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
  /// Text of prepared query
  PGSTD::string definition;
  /// Has this prepared statement been prepared in the current session?
  bool registered;

  prepared_def();
  explicit prepared_def(const PGSTD::string &);
};

} // namespace pqxx::prepare::internal
} // namespace pqxx::prepare
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"

#endif

