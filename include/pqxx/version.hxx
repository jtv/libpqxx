/* Version info for libpqxx.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/version instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#if !defined(PQXX_H_VERSION)
#  define PQXX_H_VERSION

#  if !defined(PQXX_HEADER_PRE)
#    error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#  endif

#  include <string_view>


namespace pqxx
{
/// Full libpqxx version string.
constexpr inline std::string_view const version{"8.0.0-rc5"};


/// Libpqxx ABI version string: major and minor version, but no patch.
/** For example, for libpqxx 9.3.1, this will be "9.3".
 *
 * @warning Many factors can influence the library's ABI, including compiler,
 * compiler version, and compilation options.  An ABI can also change radically
 * during release candidate development.  So don't count fully on it.
 */
constexpr inline std::string_view const abi_version{"8.0"};


/// Major libpqxx version number.  (E.g. for libpqxx 9.3.1, this will be 9.)
constexpr inline int const version_major{8};


/// Minor libpqxx version number.  (E.g. for libpqxx 9.3.1, this will be 3.)
constexpr inline int const version_minor{0};


/// Libpqxx patch version number.  (E.g. for libpqxx 9.3.1, this will be 1.)
/** For "special" versions such as pre-releases, the last component of the
 * version string is not a simple number, and `version_patch` will be -1.
 */
constexpr inline int const version_patch{-1};
} // namespace pqxx


/// Full libpqxx version string.  @deprecated Use @ref pqxx::version instead.
#  define PQXX_VERSION "8.0.0-rc5"
/// Library ABI version.  @deprecated Use @ref pqxx::abi_version instead.
#  define PQXX_ABI "8.0"
/// Major version number.  @deprecated Use @ref pqxx::version_major instead.
#  define PQXX_VERSION_MAJOR 8
/// Minor version number.  @deprecated Use @ref pqxx::version_minor instead.
#  define PQXX_VERSION_MINOR 0
#endif
