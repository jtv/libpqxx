/** Standard library symbols that we need to export for Windows DLL build.
 *
 * Apparently DLLs on Windows can't just throw or return standard library types
 * such as @c std::string or @c std::runtime_error.  The library needs to
 * export those explicitly.
 */
#if !defined(PQXX_H_EXPORTS) && defined(_WIN32) && defined(PQXX_SHARED)
#  define PQXX_H_EXPORTS

#  include <cstddef>
#  include <memory>
#  include <stdexcept>
#  include <string>
#  include <string_view>
#  include <vector>

#  include "pqxx/types.hxx"


PQXX_DLL_EXTERN template class PQXX_LIBEXPORT
  std::basic_string<char, std::char_traits<char>, std::allocator<char>>;
PQXX_DLL_EXTERN template class PQXX_LIBEXPORT std::basic_string_view<char>;
PQXX_DLL_EXTERN class PQXX_LIBEXPORT std::domain_error;
PQXX_DLL_EXTERN class PQXX_LIBEXPORT std::invalid_argument;
PQXX_DLL_EXTERN class PQXX_LIBEXPORT std::logic_error;
PQXX_DLL_EXTERN class PQXX_LIBEXPORT std::out_of_range;
PQXX_DLL_EXTERN class PQXX_LIBEXPORT std::runtime_error;
PQXX_DLL_EXTERN template class PQXX_LIBEXPORT std::shared_ptr<std::string>;
PQXX_DLL_EXTERN template class PQXX_LIBEXPORT std::vector < pqxx::format,
  std::allocator<pqxx::format>>;
PQXX_DLL_EXTERN template class PQXX_LIBEXPORT std::vector < char const *,
  std::allocator<char const *>>;
PQXX_DLL_EXTERN template class PQXX_LIBEXPORT std::vector < int, std::allocator<int>>;
PQXX_DLL_EXTERN template class PQXX_LIBEXPORT std::vector < std::string_view,
  std::allocator<std::string_view>>;

#endif
