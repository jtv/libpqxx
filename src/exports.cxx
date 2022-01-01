/** Standard library symbols that we need to export for Windows DLL build.
 *
 * Apparently DLLs on Windows can't just throw or return standard library types
 * such as @c std::string or @c std::runtime_error.  The library needs to
 * export those explicitly.
 */
#include "pqxx-source.hxx"

#if defined(_WIN32) && defined(PQXX_SHARED)
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

template class PQXX_LIBEXPORT std::basic_string<char>;
template class PQXX_LIBEXPORT std::basic_string<std::byte>;
template class PQXX_LIBEXPORT std::basic_string_view<char>;
template class PQXX_LIBEXPORT std::basic_string_view<std::byte>;
template class PQXX_LIBEXPORT std::vector<std::string_view>;

#endif
