/** Standard library symbols that we need to export for Windows DLL build.
 *
 * Apparently DLLs on Windows can't just throw or return standard library types
 * such as @c std::string or @c std::runtime_error.  The library needs to
 * export those explicitly.
 */
#include "pqxx-source.hxx"

#include <string>
#include <string_view>
#include <vector>


#if defined(_WIN32) && defined(PQXX_SHARED)
#  define PQXX_EXPORT_TYPE(type) template class PQXX_LIBEXPORT type

PQXX_EXPORT_STD(std::string);
PQXX_EXPORT_STD(std::string_view);
PQXX_EXPORT_STD(std::vector<std::string_view>);

#  undef PQXX_EXPORT_TYPE
#endif
