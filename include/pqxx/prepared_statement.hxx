/** Helper classes for defining and executing prepared statements.
 *
 * See the connection_base hierarchy for more about prepared statements.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_PREPARED_STATEMENT
#define PQXX_H_PREPARED_STATEMENT

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/types.hxx"
#include "pqxx/internal/statement_parameters.hxx"



namespace pqxx
{
/// Dedicated namespace for helper types related to prepared statements.
namespace prepare
{
/// Pass a number of statement parameters only known at runtime.
/** When you call any of the @c exec_params functions, the number of arguments
 * is normally known at compile time.  This helper function supports the case
 * where it is not.
 *
 * Use this function to pass a variable number of parameters, based on a
 * sequence ranging from @c begin to @c end exclusively.
 *
 * The technique combines with the regular static parameters.  You can use it
 * to insert dynamic parameter lists in any place, or places, among the call's
 * parameters.  You can even insert multiple dynamic sequences.
 *
 * @param begin A pointer or iterator for iterating parameters.
 * @param end A pointer or iterator for iterating parameters.
 * @return An object representing the parameters.
 */
template<typename IT> inline pqxx::internal::dynamic_params<IT>
make_dynamic_params(IT begin, IT end)
{
  return pqxx::internal::dynamic_params<IT>(begin, end);
}


/// Pass a number of statement parameters only known at runtime.
/** When you call any of the @c exec_params functions, the number of arguments
 * is normally known at compile time.  This helper function supports the case
 * where it is not.
 *
 * Use this function to pass a variable number of parameters, based on a
 * container of parameter values.
 *
 * The technique combines with the regular static parameters.  You can use it
 * to insert dynamic parameter lists in any place, or places, among the call's
 * parameters.  You can even insert multiple dynamic containers.
 *
 * @param container A container of parameter values.
 * @return An object representing the parameters.
 */
template<typename C>
inline pqxx::internal::dynamic_params<typename C::const_iterator>
make_dynamic_params(const C &container)
{
  return pqxx::internal::dynamic_params<typename C::const_iterator>(container);
}
} // namespace prepare
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"
#endif
