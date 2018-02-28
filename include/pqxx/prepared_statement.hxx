/** Helper classes for defining and executing prepared statements.
 *
 * See the connection_base hierarchy for more about prepared statements.
 *
 * Copyright (c) 2006-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_PREPARED_STATEMENT
#define PQXX_H_PREPARED_STATEMENT

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/internal/statement_parameters.hxx"

#include "pqxx/types.hxx"


namespace pqxx
{
/// Dedicated namespace for helper types related to prepared statements.
namespace prepare
{
/// Helper class for passing parameters to, and executing, prepared statements
/** @deprecated As of 6.0, use @c transaction_base::exec_prepared and friends.
 */
class PQXX_LIBEXPORT invocation : internal::statement_parameters
{
public:
  invocation(transaction_base &, const std::string &statement);
  invocation &operator=(const invocation &) =delete;

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
   * @param nonnull determines whether to pass a real value, or nullptr.
   */
  invocation &operator()(const binarystring &v, bool nonnull)
	{ add_binary_param(v, nonnull); return *this; }

  /// Pass C-style parameter string, or null if pointer is null.
  /**
   * This version is for passing C-style strings; it's a template, so any
   * pointer type that @c to_string accepts will do.
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
  transaction_base &m_home;
  const std::string m_statement;

  invocation &setparam(const std::string &, bool nonnull);
};


namespace internal
{
/// Internal representation of a prepared statement definition.
struct PQXX_LIBEXPORT prepared_def
{
  /// Text of prepared query.
  std::string definition;
  /// Has this prepared statement been prepared in the current session?
  bool registered = false;

  prepared_def() =default;
  explicit prepared_def(const std::string &);
};

} // namespace pqxx::prepare::internal
} // namespace pqxx::prepare
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"
#endif
