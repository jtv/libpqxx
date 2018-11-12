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

#include <memory>
#include <type_traits>

#if defined(PQXX_HAVE_OPTIONAL)
#include <optional>
#elif defined(PQXX_HAVE_EXP_OPTIONAL) && !defined(PQXX_HIDE_EXP_OPTIONAL)
#include <experimental/optional>
#endif


namespace pqxx
{
namespace internal
{

/// Replicate std::void_t<> (available in C++17)
template<typename... T> using void_t = void;

/// Shortcut for detecting type held by an optional-like wrapper type
template<typename T> using inner_type = typename std::remove_reference<
  decltype(*std::declval<T>())
>::type;

/// Detect if the given type has an `operator *()`
template<typename T, typename = void> struct is_derefable : std::false_type {};
template<typename T> struct is_derefable<T, void_t<
  // Disable for arrays so they don't erroneously decay to pointers
  inner_type<typename std::enable_if<!std::is_array<T>::value, T>::type>
>> : std::true_type {};

/// Detect if the given type has an explicit pqxx::string_traits specialization
template<typename T, typename = void> struct has_string_traits : std::false_type {};
template<typename T> struct has_string_traits<T, void_t<
  decltype(pqxx::string_traits<T>::name),
  decltype(pqxx::string_traits<T>::has_null),
  decltype(pqxx::string_traits<T>::is_null),
  decltype(pqxx::string_traits<T>::null),
  decltype(pqxx::string_traits<T>::from_string),
  decltype(pqxx::string_traits<T>::to_string)
>> : std::true_type {};

/// Detect if the given type should be treated as an optional-value wrapper type
template<typename T, typename = void> struct is_optional : std::false_type {};
template<typename T> struct is_optional<T, typename std::enable_if<
  (
    is_derefable<T>::value
    // Check if an `explicit operator bool` exists for this type
    && std::is_constructible<bool, T>::value 
    // Disable if an explicit `pqxx::string_traits<>` exists for this type
    && !has_string_traits<T>::value
  ),
  void
>::type> : std::true_type {};

/// Detect if `std::nullopt_t`/`std::experimental::nullopt_t` can be implicitly
/// converted to the given type
template<
  typename T,
  typename = void
> struct takes_std_nullopt : std::false_type {};
#if defined(PQXX_HAVE_OPTIONAL)
template<typename T> struct takes_std_nullopt<
    T,
    typename std::enable_if<
      std::is_assignable<T, std::nullopt_t>::value,
      void
    >::type
> : std::true_type {};
#elif defined(PQXX_HAVE_EXP_OPTIONAL) && !defined(PQXX_HIDE_EXP_OPTIONAL)
template<typename T> struct takes_std_nullopt<
    T,
    typename std::enable_if<
      std::is_assignable<T, std::experimental::nullopt_t>::value,
      void
    >::type
> : std::true_type {};
#endif

/// Detect if type is a `std::tuple<>`
template<typename T, typename = void> struct is_tuple : std::false_type {};
template<typename T> struct is_tuple<
  T,
  typename std::enable_if<(std::tuple_size<T>::value >= 0), void>::type
> : std::true_type {};

/// Detect if a type is an iterable container
template<typename T, typename = void> struct is_container : std::false_type {};
template<typename T> struct is_container<
  T,
  void_t<
    decltype(std::begin(std::declval<T>())),
    decltype(std::end(std::declval<T>())),
    // Some people might implement a `std::tuple<>` specialization that is
    // iterable when all the contained types are the same ;)
    typename std::enable_if<!is_tuple<T>::value, void>::type
  >
> : std::true_type {};

/// Get an appropriate null value for the given type
/**
 *  pointer types                         `nullptr`
 *  `std::optional<>`-like                `std::nullopt`
 *  `std::experimental::optional<>`-like  `std::experimental::nullopt`
 *  other types                           `pqxx::string_traits<>::null()`
 */
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    (is_optional<T>::value && !takes_std_nullopt<T>::value),
    std::nullptr_t
  >::type
{ return nullptr; }
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    (!is_optional<T>::value && !takes_std_nullopt<T>::value),
    decltype(pqxx::string_traits<T>::null())
  >::type
{ return pqxx::string_traits<T>::null(); }
#if defined(PQXX_HAVE_OPTIONAL)
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    takes_std_nullopt<T>::value,
    std::nullopt_t
  >::type
{ return std::nullopt; }
#elif defined(PQXX_HAVE_EXP_OPTIONAL) && !defined(PQXX_HIDE_EXP_OPTIONAL)
template<typename T> constexpr auto null_value()
  -> typename std::enable_if<
    takes_std_nullopt<T>::value,
    std::experimental::nullopt_t
  >::type
{ return std::experimental::nullopt; }
#endif

/// Construct an optional-like type from the stored type
/** 
 * While these may seem redundant, they are necessary to support smart pointers
 * as optional storage types in a generic manner.
 * Users may add support for their own pointer types following this pattern.
 */
// Enabled if the wrapper type can be directly constructed from the wrapped type
// (e.g. std::optional)
template<typename T> constexpr auto make_optional(inner_type<T> v)
  -> decltype(T(v))
{ return T(v); }
// Enabled if T is a specialization of std::unique_ptr<>
template<typename T> constexpr auto make_optional(inner_type<T> v)
  -> typename std::enable_if<
    std::is_same<T, std::unique_ptr<inner_type<T>>>::value,
    std::unique_ptr<inner_type<T>>
  >::type
{ return std::make_unique<inner_type<T>>(v); }
// Enabled if T is a specialization of std::shared_ptr<>
template<typename T> constexpr auto make_optional(inner_type<T> v)
  -> typename std::enable_if<
    std::is_same<T, std::shared_ptr<inner_type<T>>>::value,
    std::shared_ptr<inner_type<T>>
  >::type
{ return std::make_shared<inner_type<T>>(v); }

} // namespace pqxx::internal
} // namespace pqxx


// TODO: Move?
namespace pqxx
{

/// Meta `pqxx::string_traits` for optional types
template<typename T> struct string_traits<
  T,
  typename std::enable_if<internal::is_optional<T>::value, void>::type
>
{
private:
  using I = internal::inner_type<T>;
public:
  static constexpr const char *name() noexcept
    { return string_traits<I>::name(); }
  static constexpr bool has_null() noexcept { return true; }
  static bool is_null(const T& v)
    { return (!v || string_traits<I>::is_null(*v)); }
  static constexpr T null() { return internal::null_value<T>(); }
  static void from_string(const char Str[], T &Obj)
  {
    if (!Str) Obj = null();
    else
    {
      I inner;
      string_traits<I>::from_string(Str, inner);
      Obj = inner;
    }
  }
  static std::string to_string(const T& Obj)
  {
    if (is_null(Obj)) internal::throw_null_conversion(name());
    return string_traits<I>::to_string(*Obj);
  }
};

} // namespace pqxx


#endif
