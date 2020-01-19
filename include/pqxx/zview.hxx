/* Zero-terminated string view.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stringconv instead.
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

namespace pqxx
{
/// Marker-type wrapper: zero-terminated @c std::string_view.
/** @warning Use this only if the underlying string is zero-terminated.
 *
 * When you construct a zview, you are promising that the underlying string is
 * zero-terminated.  It otherwise behaves exactly like a std::string_view.
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
  template<typename... Args>
  explicit constexpr zview(Args &&... args) :
          std::string_view(std::forward<Args>(args)...)
  {}

  /// Either a null pointer, or a zero-terminated text buffer.
  [[nodiscard]] constexpr char const *c_str() const noexcept { return data(); }
};
} // namespace pqxx

#endif
