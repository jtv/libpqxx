/** Type/template metaprogramming utilities for use internally in libpqxx
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TYPE_UTILS
#define PQXX_H_TYPE_UTILS

#include <type_traits>

#ifdef PQXX_HAVE_OPTIONAL
#include <optional>
#elif defined PQXX_HAVE_EXP_OPTIONAL
#include <experimental/optional>
#endif


namespace pqxx
{
namespace internal
{

// std::void_t<> available in C++17
template<typename... T> using void_t = void;

// Detect if the given type has an `operator *()`
template<typename T, typename = void> struct is_derefable : std::false_type {};
template<typename T> struct is_derefable<T, void_t<
  decltype(*(T{}))
>> : std::true_type {};

// Detect if `std::nullopt_t`/`std::experimental::nullopt_t` can be implicitly
// converted to the given type
template<
  typename T,
  typename = void
> struct takes_std_nullopt : std::false_type {};
#ifdef PQXX_HAVE_OPTIONAL
template<typename T> struct takes_std_nullopt<
    T,
    typename std::enable_if<
      std::is_assignable<T, std::nullopt_t>::value,
      void
    >::type
> : std::true_type {};
#elif defined PQXX_HAVE_EXP_OPTIONAL
template<typename T> struct takes_std_nullopt<
    T,
    typename std::enable_if<
      std::is_assignable<T, std::experimental::nullopt_t>::value,
      void
    >::type
> : std::true_type {};
#endif

/** Get an appropriate null value for the given type
 *
 *  pointer types                         `nullptr`
 *  `std::optional<>`-like                `std::nullopt`
 *  `std::experimental::optional<>`-like  `std::experimental::nullopt`
 *  other types                           `pqxx::string_traits<>::null()`
 */
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    (is_derefable<T>::value && !takes_std_nullopt<T>::value),
    std::nullptr_t
  >::type
{ return nullptr; }
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    (!is_derefable<T>::value && !takes_std_nullopt<T>::value),
    decltype(pqxx::string_traits<T>::null())
  >::type
{ return pqxx::string_traits<T>::null(); }
#ifdef PQXX_HAVE_OPTIONAL
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    takes_std_nullopt<T>::value,
    std::nullopt_t
  >::type
{ return std::nullopt; }
#elif defined PQXX_HAVE_EXP_OPTIONAL
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    takes_std_nullopt<T>::value,
    std::experimental::nullopt_t
  >::type
{ return std::experimental::nullopt; }
#endif

} // namespace pqxx::internal
} // namespace pqxx


#endif
