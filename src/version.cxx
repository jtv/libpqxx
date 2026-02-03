/** Version check.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include "pqxx/internal/header-pre.hxx"

#include <format>

#include "pqxx/except.hxx"
#include "pqxx/version.hxx"

#include "pqxx/internal/header-post.hxx"


namespace pqxx::internal
{
PQXX_NOINLINE
int check_libpqxx_version(
  int apps_major, int apps_minor, int apps_patch,
  std::string_view apps_version)
{
  if ((apps_major == version_major) and (apps_minor == version_minor))
  {
    // Major and minor versions match.  Without that, the odds of even getting
    // to this point in the code are pretty slim anyway.

    if (0 <= apps_patch)
    {
      // Application was compiled against a "regular" libpqxx version.  We're
      // cool so long as the libpqxx binary is also regular, and the binary's
      // patch level is no less than the one used in the application build.
      //
      // (If 0 <= aops_patch <= patch, then it follows that 0 <= patch.  No
      // need to check that.)
      if (apps_patch <= version_patch)
        return 1;
    }
    else
    {
      // Compiled against a "special" libpqxx version, such as a release
      // candidate.  We don't try to get clever about tolerating mismatches in
      // such cases; there's just too much that can go wrong.  Demand an exact
      // string match.
      if (apps_version == version)
        return 2;
    }
  }

  throw version_mismatch{std::format(
    "Mismatch in libpqxx versions: Compiled against libpqxx {} headers, "
    "but libpqxx binary is {}.",
    apps_version, version)};
}
} // namespace pqxx::internal
