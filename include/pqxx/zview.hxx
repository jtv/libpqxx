/* Zero-terminated string view.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/zview instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ZVIEW
#define PQXX_H_ZVIEW

#include <cassert>
#include <string>
#include <string_view>
#include <type_traits>

#include "pqxx/types.hxx"


namespace pqxx
{
class zview;


/// Concept: T is a known zero-terminated string type.
/** There's no unified API for these string types.  It's just a check for some
 * known types.  Any code that makes use of the concept will still have to
 * support each of these individually.
 */
template<typename T>
concept ZString =
  (std::is_convertible_v<std::remove_cvref_t<T>, char const *> or
   std::is_convertible_v<std::remove_cvref_t<T>, zview> or
   std::is_convertible_v<T, std::string const &>) and
  not std::is_convertible_v<T, std::nullptr_t>;


/// Marker-type wrapper: zero-terminated `std::string_view`.
/** @warning Use this only if the underlying string is zero-terminated.
 *
 * When you construct a zview, you are promising that the data pointer is
 * non-null, and the underlying string is zero-terminated.  It otherwise
 * behaves exactly like a `std::string_view`.
 *
 * The terminating zero is not "in" the string, so it does not count as part of
 * the view's length.
 *
 * The added guarantee lets the view be used as a C-style string, which often
 * matters since libpqxx builds on top of a C library.  For this reason, zview
 * also adds a @ref c_str method.
 */
class PQXX_LIBEXPORT zview final : public std::string_view
{
public:
  /// Default constructor produces a zero-terminated empty string.
  constexpr zview() noexcept : zview{""} {}

  /// Convenience overload: construct using pointer and signed length.
  /** Even though you specify the length, there must still be a zero byte just
   * beyond that length, at `text[len]`.
   */
  PQXX_ZARGS constexpr zview(char const text[], std::ptrdiff_t len) noexcept(
    noexcept(std::string_view{text, static_cast<std::size_t>(len)})) :
          std::string_view{text, static_cast<std::size_t>(len)}
  {
    invariant();
  }

  /// Convenience overload: construct using pointer and signed length.
  constexpr zview(char text[], std::ptrdiff_t len) noexcept(
    noexcept(std::string_view{text, static_cast<std::size_t>(len)})) :
          std::string_view{text, static_cast<std::size_t>(len)}
  {
    invariant();
  }

  /// Explicitly promote a `string_view` to a `zview`.
  /** @warning This is not just a type conversion.  It's the caller making a
   * promise that the string is zero-terminated.
   */
  explicit constexpr zview(std::string_view other) noexcept :
          std::string_view{other}
  {
    invariant();
  }

  /// Construct from any initialiser you might use for `std::string_view`.
  /** @warning Only do this if you are sure that the string is zero-terminated.
   */
  template<typename... Args>
  explicit constexpr zview(Args &&...args) :
          std::string_view(std::forward<Args>(args)...)
  {
    invariant();
  }

  /// @warning There's an implicit conversion from `std::string`.
  constexpr zview(std::string const &str) noexcept :
          std::string_view{str.c_str(), str.size()}
  {
    invariant();
  }

  /// Construct a `zview` from a C-style string.
  /** @warning This scans the string to discover its length.  So if you need to
   * do it many times, it's probably better to create the `zview` once and
   * re-use it.
   */
  PQXX_ZARGS constexpr zview(char const str[]) noexcept(
    noexcept(std::string_view{str})) :
          std::string_view{str}
  {
    invariant();
  }

  zview(std::nullptr_t) = delete;

  /// Construct a `zview` from a string literal.
  /** A C++ string literal ("foo") normally looks a lot like a pointer to
   * char const, but that's not really true.  It's actually an array of char,
   * which _devolves_ to a pointer when you pass it.
   *
   * For the purpose of creating a `zview` there is one big difference: if we
   * know the array's size, we don't need to scan through the string in order
   * to find out its length.
   */
  template<size_t size>
  PQXX_ZARGS constexpr zview(char const (&literal)[size]) :
          zview(literal, size - 1)
  {}

  /// Return as C string.
  [[nodiscard]] constexpr char const *c_str() const & noexcept
  {
    return data();
  }

  /// Return as C string.
  constexpr operator char const *() const noexcept { return data(); }

  /// Disambiguating comparison operator: leave it to `std::string_view`.
  constexpr bool operator==(zview const &rhs) const noexcept
  {
    return std::string_view{*this} == std::string_view{rhs};
  }
  /// Disambiguating comparison operator: leave it to `std::string_view`.
  constexpr bool operator!=(zview const &rhs) const noexcept
  {
    return std::string_view{*this} != std::string_view{rhs};
  }

private:
  /// Check invariant: `data()` must be non-null and zero-terminated.
  [[maybe_unused]] constexpr void invariant() const noexcept
  {
    assert(std::data(*this) != nullptr);
    assert(std::data(*this)[std::size(*this)] == '\0');
  }
};


template<> inline constexpr std::string_view name_type<zview>() noexcept
{
  return "pqxx::zview";
}


/// Support @ref zview literals.
/** You can "import" this selectively into your namespace, without pulling in
 * all of the @ref pqxx namespace:
 *
 * ```cxx
 * using pqxx::operator"" _zv;
 * ```
 */
PQXX_ZARGS constexpr zview
operator""_zv(char const str[], std::size_t len) noexcept
{
  return zview{str, len};
}


/// Disambiguating comparison operator: leave it to `std::string_view`.
constexpr inline bool operator==(char const lhs[], pqxx::zview rhs) noexcept
{
  return std::string_view{lhs} == std::string_view{rhs};
}


/// Disambiguating comparison operator: leave it to `std::string_view`.
constexpr inline bool operator!=(char const lhs[], pqxx::zview rhs) noexcept
{
  return std::string_view{lhs} != std::string_view{rhs};
}
} // namespace pqxx


/// A zview is a view.
template<> inline constexpr bool std::ranges::enable_view<pqxx::zview>{true};


/// A zview is a borrowed range.
template<>
inline constexpr bool std::ranges::enable_borrowed_range<pqxx::zview>{true};
#endif
