/* Version info for libpqxx.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/version instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_VERSION

#  include "pqxx/compiler-public.hxx"
#  include "pqxx/internal/compiler-internal-pre.hxx"

/// Full libpqxx version string.
#  define PQXX_VERSION "7.2.0"
/// Library ABI version.
#  define PQXX_ABI "7.2"

/// Major version number.
#  define PQXX_VERSION_MAJOR 7
/// Minor version number.
#  define PQXX_VERSION_MINOR 2

#  define PQXX_VERSION_CHECK                                                  \
    check_pqxx_version_##PQXX_VERSION_MAJOR##_##PQXX_VERSION_MINOR

namespace pqxx::internal
{
/// Library version check stub.
/** Helps detect version mismatches between libpqxx headers and the libpqxx
 * library binary.
 *
 * Sometimes users run into trouble linking their code against libpqxx because
 * they build their own libpqxx, but the system also has a different version
 * installed.  The declarations in the headers against which they compile their
 * code will differ from the ones used to build the libpqxx version they're
 * using, leading to confusing link errors.  The solution is to generate a link
 * error when the libpqxx binary is not the same version as the libpqxx headers
 * used to compile the code.
 *
 * This function's definition is in the libpqxx binary, so it's based on the
 * version as found in the binary.  The headers contain a call to the function,
 * whose name contains the libpqxx version as found in the headers.  (The
 * library build process will use its own local headers even if another version
 * of the headers is installed on the system.)
 *
 * If the libpqxx binary was compiled for a different version than the user's
 * code, linking will fail with an error: @c check_library_version will not
 * exist for the given version number.
 */
PQXX_LIBEXPORT int PQXX_VERSION_CHECK() noexcept;
} // namespace pqxx::internal
#  include "pqxx/internal/compiler-internal-post.hxx"
#endif
