/** Type/template metaprogramming utilities for use internally in libpqxx
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TYPE_UTILS
#define PQXX_H_TYPE_UTILS

#include <optional>
#include <type_traits>


namespace pqxx::internal
{
// TODO: Replace nested `std::remove_*`s with `std::remove_cvref` in C++20.
/// Extract the content type held by an `optional`-like wrapper type.
template<typename T> using inner_type = typename std::remove_cv<
  typename std::remove_reference<
    decltype(*std::declval<T>())
  >::type
>::type;

/// Does the given type have an `operator *()`?
template<typename T, typename = void> struct is_derefable : std::false_type {};
template<typename T> struct is_derefable<T, std::void_t<
  // Disable for arrays so they don't erroneously decay to pointers.
  inner_type<typename std::enable_if<not std::is_array<T>::value, T>::type>
>> : std::true_type {};

/// Should the given type be treated as an optional-value wrapper type?
template<typename T, typename = void> struct is_optional : std::false_type {};
template<typename T> struct is_optional<T, typename std::enable_if_t<(
  is_derefable<T>::value and
  // Check if an `explicit operator bool` exists for this type
  std::is_constructible<bool, T>::value
)>> : std::true_type {};

/// Can `nullopt_t` implicitly convert to type T?
template<
  typename T,
  typename = void
> struct takes_std_nullopt : std::false_type {};

template<typename T> struct takes_std_nullopt<
    T,
    typename std::enable_if<std::is_assignable<T, std::nullopt_t>::value>::type
> : std::true_type {};

/// Is type T a `std::tuple<>`?
template<typename T, typename = void> struct is_tuple : std::false_type {};
template<typename T> struct is_tuple<
  T,
  typename std::enable_if<(std::tuple_size<T>::value >= 0)>::type
> : std::true_type {};

/// Is type T an iterable container?
template<typename T, typename = void> struct is_container : std::false_type {};
template<typename T> struct is_container<
  T,
  std::void_t<
    decltype(std::begin(std::declval<T>())),
    decltype(std::end(std::declval<T>())),
    // Some people might implement a `std::tuple<>` specialization that is
    // iterable when all the contained types are the same ;)
    typename std::enable_if<not is_tuple<T>::value>::type
  >
> : std::true_type {};

/// Get an appropriate null value for the given type.
/**
 * pointer types                         `nullptr`
 * `std::optional<>`-like                `std::nullopt`
 * `std::experimental::optional<>`-like  `std::experimental::nullopt`
 * other types                           `pqxx::string_traits<>::null()`
 * Users may add support for their own wrapper types following this pattern.
 */
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    (
      is_optional<T>::value and
      not takes_std_nullopt<T>::value and
      std::is_assignable<T, std::nullptr_t>::value
    ),
    std::nullptr_t
  >::type
{ return nullptr; }

template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    (not is_optional<T>::value and not takes_std_nullopt<T>::value),
    decltype(pqxx::string_traits<T>::null())
  >::type
{ return pqxx::string_traits<T>::null(); }

template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    takes_std_nullopt<T>::value,
    std::nullopt_t
  >::type
{ return std::nullopt; }
} // namespace pqxx::internal
#endif
