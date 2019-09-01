/* Helper classes for defining and executing prepared statements.
 *
 * See the connection class for more about prepared statements.
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


/// Dedicated namespace for helper types related to prepared statements.
namespace pqxx::prepare
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
template<typename IT>
inline auto
make_dynamic_params(
	IT begin,
	IT end)
{
  return pqxx::internal::dynamic_params(begin, end);
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
inline auto
make_dynamic_params(const C &container)
{
  using IT = typename C::const_iterator;
  return pqxx::internal::dynamic_params<IT>{container};
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
 * @param accessor For each parameter @c p, pass @c accessor(p).
 * @return An object representing the parameters.
 */
template<typename C, typename ACCESSOR>
inline auto
make_dynamic_params(
	C &container,
	ACCESSOR accessor)
{
  using IT = decltype(std::begin(container));
  return pqxx::internal::dynamic_params<IT, ACCESSOR>{container, accessor};
}
} // namespace pqxx::prepare

#include "pqxx/compiler-internal-post.hxx"
#endif
