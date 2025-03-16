/** Implementation of types-related helpers.
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#if __has_include(<cxxabi.h>)
#  include <cxxabi.h>
#endif

#include "pqxx/internal/header-pre.hxx"
#include "pqxx/types.hxx"
#include "pqxx/internal/header-post.hxx"


std::string pqxx::internal::demangle_type_name(char const raw[])
{
#if defined(PQXX_HAVE_CXA_DEMANGLE)
  // We've got __cxa_demangle.  Use it to get a friendlier type name.
  int status{0};
  std::size_t len{0};

  // We've seen this fail on FreeBSD 11.3 (see #361).  Trying to throw a
  // meaningful exception only made things worse.  So in case of error, just
  // fall back to the raw name.
  //
  // When __cxa_demangle fails, it's guaranteed to return null.
  char *str{abi::__cxa_demangle(raw, nullptr, &len, &status)};

  if (str)
  {
    try
    {
      std::string out{str};
      // NOLINTNEXTLINE(*-no-malloc,cppcoreguidelines-owning-memory)
      std::free(str);
      str = nullptr;
      return out;
    }
    catch (std::exception const &)
    {
      // NOLINTNEXTLINE(*-no-malloc,cppcoreguidelines-owning-memory)
      std::free(str);
      throw;
    }
  }
#endif
  return raw;
}
