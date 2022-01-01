/** Standard library symbols that we need to export for Windows DLL build.
 *
 * Apparently DLLs on Windows can't just throw or return standard library types
 * such as @c std::string or @c std::runtime_error.  The library needs to
 * export those explicitly.
 */
#include "pqxx-source.hxx"

#if defined(_WIN32) && defined(PQXX_SHARED)
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "pqxx/types.hxx"


template class PQXX_LIBEXPORT std::basic_string<char,std::char_traits<char>,std::allocator<char>>;
template class PQXX_LIBEXPORT std::basic_string_view<char>;
class PQXX_LIBEXPORT std::domain_error;
class PQXX_LIBEXPORT std::invalid_argument;
class PQXX_LIBEXPORT std::logic_error;
class PQXX_LIBEXPORT std::out_of_range;
class PQXX_LIBEXPORT std::runtime_error;
template class PQXX_LIBEXPORT std::shared_ptr<std::string>;
template class PQXX_LIBEXPORT std::vector<pqxx::format,std::allocator<pqxx::format>;
template class PQXX_LIBEXPORT std::vector<char const *,std::allocator<char const *>;
template class PQXX_LIBEXPORT std::vector<int,std::allocator<int>;
template class PQXX_LIBEXPORT std::vector<std::string_view,std::allocator<std::string_view>;

#endif
