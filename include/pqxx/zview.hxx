/* Zero-terminated string view.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/zview instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ZVIEW
#define PQXX_H_ZVIEW

#include "pqxx/compiler-public.hxx"

#include <string_view>
#include <type_traits>


namespace pqxx
{
/// Marker-type wrapper: zero-terminated @c std::string_view.
/** @warning Use this only if the underlying string is zero-terminated.
 *
 * When you construct a zview, you are promising that if the data pointer is
 * non-null, the underlying string is zero-terminated.  It otherwise behaves
 * exactly like a std::string_view.
 *
 * The terminating zero is not "in" the string, so it does not count as part of
 * the view's length.
 *
 * The added guarantee lets the view be used as a C-style string, which often
 * matters since libpqxx builds on top of a C library.  For this reason, zview
 * also adds a @c c_str method.
 */
class zview : public std::string_view
{
public:
  constexpr zview() noexcept = default;

  /// Convenience overload: construct using pointer and signed length.
  constexpr zview(char const text[], std::ptrdiff_t len) :
          std::string_view{text, static_cast<std::size_t>(len)}
  {}

  /// Convenience overload: construct using pointer and signed length.
  constexpr zview(char text[], std::ptrdiff_t len) :
          std::string_view{text, static_cast<std::size_t>(len)}
  {}

  /// Construct from any initialiser you might use for @c std::string_view.
  /** @warning Only do this if you are sure that the string is zero-terminated.
   */
  template<typename... Args>
  explicit constexpr zview(Args &&... args) :
          std::string_view(std::forward<Args>(args)...)
  {}

  /* implicit */
  zview(std::string const& str) :
          std::string_view(str)
  {}

  /* implicit */
  template<size_t size>
  constexpr zview(char const (&literal)[size]) :
          zview(literal, size - 1)
  {}

  /// Either a null pointer, or a zero-terminated text buffer.
  [[nodiscard]] constexpr char const *c_str() const noexcept { return data(); }
};
} // namespace pqxx


#if defined(PQXX_HAVE_CONCEPTS)
namespace pqxx::internal
{
/// Concept: T is a known zero-terminated string type.
/** There's no unified API for these string types.  It's just a check for some
 * known types.  Any code that makes use of the concept will still have to
 * support each of these individually.
 */
template<typename T>
concept ZString =
  std::is_convertible_v<T, char const *> or std::is_convertible_v<T, zview> or
  std::is_convertible_v<T, std::string const &>;
} // namespace pqxx::internal
#endif // PQXX_HAVE_CONCEPTS


namespace pqxx::internal
{
/// Get a raw C string pointer.
inline constexpr char const *as_c_string(char const str[]) noexcept
{
  return str;
}
/// Get a raw C string pointer.
template<std::size_t N>
inline constexpr char const *as_c_string(char (&str)[N]) noexcept
{
  return str;
}
/// Get a raw C string pointer.
inline constexpr char const *as_c_string(pqxx::zview str) noexcept
{
  return str.c_str();
}
// TODO: constexpr as of C++20.
/// Get a raw C string pointer.
inline char const *as_c_string(std::string const &str) noexcept
{
  return str.c_str();
}
} // namespace pqxx::internal

#endif
