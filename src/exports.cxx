/** Standard library symbols that we need to export for Windows DLL build.
 *
 * Apparently DLLs on Windows can't just throw or return standard library types
 * such as @c std::string or @c std::runtime_error.  The library needs to
 * export those explicitly.
 */
#include "pqxx-source.hxx"

#if defined(_WIN32) && defined(PQXX_SHARED)
#  include "pqxx/internal/exports.hxx"
#endif
