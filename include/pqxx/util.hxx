/* Various utility definitions for libpqxx.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/util instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_UTIL
#define PQXX_H_UTIL

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <cctype>
#include <cstdio>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <vector>

#include "pqxx/except.hxx"
#include "pqxx/internal/encodings.hxx"
#include "pqxx/types.hxx"
#include "pqxx/version.hxx"


/// The home of all libpqxx classes, functions, templates, etc.
namespace pqxx
{}

#include <pqxx/internal/libpq-forward.hxx>


namespace pqxx
{
/// Suppress compiler warning about an unused item.
template<typename... T> inline void ignore_unused(T &&...) {}


/// Cast a numeric value to another type, or throw if it underflows/overflows.
/** Both types must be arithmetic types, and they must either be both integral
 * or both floating-point types.
 */
template<typename TO, typename FROM>
inline TO check_cast(FROM value, char const description[])
{
  static_assert(std::is_arithmetic_v<FROM>);
  static_assert(std::is_arithmetic_v<TO>);
  static_assert(std::is_integral_v<FROM> == std::is_integral_v<TO>);

  // The rest of this code won't quite work for bool, but bool is trivially
  // convertible to other arithmetic types as far as I can see.
  if constexpr (std::is_same_v<FROM, bool>)
    return static_cast<TO>(value);

  // Depending on our "if constexpr" conditions, this parameter may not be
  // needed.  Some compilers will warn.
  ignore_unused(description);

  using from_limits = std::numeric_limits<decltype(value)>;
  using to_limits = std::numeric_limits<TO>;
  if constexpr (std::is_signed_v<FROM>)
  {
    if constexpr (std::is_signed_v<TO>)
    {
      if (value < to_limits::lowest())
        throw range_error(std::string{"Cast underflow: "} + description);
    }
    else
    {
      // FROM is signed, but TO is not.  Treat this as a special case, because
      // there may not be a good broader type in which the compiler can even
      // perform our check.
      if (value < 0)
        throw range_error(
          std::string{"Casting negative value to unsigned type: "} +
          description);
    }
  }
  else
  {
    // No need to check: the value is unsigned so can't fall below the range
    // of the TO type.
  }

  if constexpr (std::is_integral_v<FROM>)
  {
    using unsigned_from = std::make_unsigned_t<FROM>;
    using unsigned_to = std::make_unsigned_t<TO>;
    constexpr auto from_max{static_cast<unsigned_from>((from_limits::max)())};
    constexpr auto to_max{static_cast<unsigned_to>((to_limits::max)())};
    if constexpr (from_max > to_max)
    {
      if (static_cast<unsigned_from>(value) > to_max)
        throw range_error(std::string{"Cast overflow: "} + description);
    }
  }
  else if constexpr ((from_limits::max)() > (to_limits::max)())
  {
    if (value > (to_limits::max)())
      throw range_error(std::string{"Cast overflow: "} + description);
  }

  return static_cast<TO>(value);
}


/// Remove any constness, volatile, and reference-ness from a type.
/** @deprecated In C++20 we'll replace this with std::remove_cvref.
 */
template<typename TYPE>
using strip_t = std::remove_cv_t<std::remove_reference_t<TYPE>>;


/** Check library version at link time.
 *
 * Ensures a failure when linking an application against a radically
 * different libpqxx version than the one against which it was compiled.
 *
 * Sometimes application builds fail in unclear ways because they compile
 * using headers from libpqxx version X, but then link against libpqxx
 * binary version Y.  A typical scenario would be one where you're building
 * against a libpqxx which you have built yourself, but a different version
 * is installed on the system.
 *
 * The check_library_version template is declared for any library version,
 * but only actually defined for the version of the libpqxx binary against
 * which the code is linked.
 *
 * If the library binary is a different version than the one declared in
 * these headers, then this call will fail to link: there will be no
 * definition for the function with these exact template parameter values.
 * There will be a definition, but the version in the parameter values will
 * be different.
 */
inline PQXX_PRIVATE void check_version()
{
  // There is no particular reason to do this here in @c connection, except
  // to ensure that every meaningful libpqxx client will execute it.  The call
  // must be in the execution path somewhere or the compiler won't try to link
  // it.  We can't use it to initialise a global or class-static variable,
  // because a smart compiler might resolve it at compile time.
  //
  // On the other hand, we don't want to make a useless function call too
  // often for performance reasons.  A local static variable is initialised
  // only on the definition's first execution.  Compilers will be well
  // optimised for this behaviour, so there's a minimal one-time cost.
  static auto const version_ok{internal::PQXX_VERSION_CHECK()};
  ignore_unused(version_ok);
}


/// Descriptor of library's thread-safety model.
/** This describes what the library knows about various risks to thread-safety.
 */
struct PQXX_LIBEXPORT thread_safety_model
{
  /// Is the underlying libpq build thread-safe?
  bool safe_libpq = false;

  /// Is Kerberos thread-safe?
  /** @warning Is currently always @c false.
   *
   * If your application uses Kerberos, all accesses to libpqxx or Kerberos
   * must be serialized.  Confine their use to a single thread, or protect it
   * with a global lock.
   */
  bool safe_kerberos = false;

  /// A human-readable description of any thread-safety issues.
  std::string description;
};


/// Describe thread safety available in this build.
[[nodiscard]] PQXX_LIBEXPORT thread_safety_model describe_thread_safety();


/// The "null" oid.
constexpr oid oid_none{0};
} // namespace pqxx


/// Private namespace for libpqxx's internal use; do not access.
/** This namespace hides definitions internal to libpqxx.  These are not
 * supposed to be used by client programs, and they may change at any time
 * without notice.
 *
 * Conversely, if you find something in this namespace tremendously useful, by
 * all means do lodge a request for its publication.
 *
 * @warning Here be dragons!
 */
namespace pqxx::internal
{
/// Helper base class: object descriptions for error messages and such.
/**
 * Classes derived from namedclass have a class name (such as "transaction"),
 * an optional object name (such as "delete-old-logs"), and a description
 * generated from the two names (such as "transaction delete-old-logs").
 *
 * The class name is dynamic here, in order to support inheritance hierarchies
 * where the exact class name may not be known statically.
 *
 * In inheritance hierarchies, make namedclass a virtual base class so that
 * each class in the hierarchy can specify its own class name in its
 * constructors.
 */
class PQXX_LIBEXPORT namedclass
{
public:
  explicit namedclass(std::string_view classname) : m_classname{classname} {}

  namedclass(std::string_view classname, std::string_view name) :
          m_classname{classname}, m_name{name}
  {}

  namedclass(std::string_view classname, char const name[]) :
          m_classname{classname}, m_name{name}
  {}

  namedclass(std::string_view classname, std::string &&name) :
          m_classname{classname}, m_name{std::move(name)}
  {}

  /// Object name, or the empty string if no name was given.
  std::string const &name() const noexcept { return m_name; }

  /// Class name.
  std::string const &classname() const noexcept { return m_classname; }

  /// Combination of class name and object name; or just class name.
  std::string description() const;

private:
  std::string m_classname, m_name;
};


PQXX_PRIVATE void check_unique_registration(
  namedclass const *new_ptr, namedclass const *old_ptr);
PQXX_PRIVATE void check_unique_unregistration(
  namedclass const *new_ptr, namedclass const *old_ptr);


/// Ensure proper opening/closing of GUEST objects related to a "host" object
/** Only a single GUEST may exist for a single host at any given time.  GUEST
 * must be derived from namedclass.
 */
template<typename GUEST> class unique
{
public:
  constexpr unique() = default;
  constexpr unique(unique const &) = delete;
  constexpr unique(unique &&rhs) : m_guest(rhs.m_guest)
  {
    rhs.m_guest = nullptr;
  }
  constexpr unique &operator=(unique const &) = delete;
  constexpr unique &operator=(unique &&rhs)
  {
    m_guest = rhs.m_guest;
    rhs.m_guest = nullptr;
    return *this;
  }

  constexpr GUEST *get() const noexcept { return m_guest; }

  constexpr void register_guest(GUEST *G)
  {
    check_unique_registration(G, m_guest);
    m_guest = G;
  }

  constexpr void unregister_guest(GUEST *G)
  {
    check_unique_unregistration(G, m_guest);
    m_guest = nullptr;
  }

private:
  GUEST *m_guest = nullptr;
};


/// Compute buffer size needed to escape binary data for use as a BYTEA.
/** This uses the hex-escaping format.  The return value includes room for the
 * "\x" prefix.
 */
constexpr std::size_t size_esc_bin(std::size_t binary_bytes) noexcept
{
  return 2 + (2 * binary_bytes) + 1;
}


/// Compute binary size from the size of its escaped version.
/** Do not include a terminating zero in @c escaped_bytes.
 */
constexpr std::size_t size_unesc_bin(std::size_t escaped_bytes) noexcept
{
  return (escaped_bytes - 2) / 2;
}


/// Hex-escape binary data into a buffer.
/** The buffer must be able to accommodate @c size_esc_bin(binary_data.size())
 * bytes, and the function will write exactly that number of bytes into the
 * buffer.  This includes a trailing zero.
 */
void PQXX_LIBEXPORT
esc_bin(std::string_view binary_data, char buffer[]) noexcept;


/// Hex-escape binary data into a std::string.
std::string PQXX_LIBEXPORT esc_bin(std::string_view binary_data);


/// Reconstitute binary data from its escaped version.
void PQXX_LIBEXPORT
unesc_bin(std::string_view escaped_data, unsigned char buffer[]);


/// Reconstitute binary data from its escaped version.
std::string PQXX_LIBEXPORT unesc_bin(std::string_view escaped_data);
} // namespace pqxx::internal

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
