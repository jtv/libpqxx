/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/compiler-internal.hxx
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds for compiling libpqxx itself.
 *      DO NOT INCLUDE THIS FILE when building client programs.
 *
 * Copyright (c) 2002-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_COMPILER_INTERNAL
#define PQXX_H_COMPILER_INTERNAL


// Workarounds & definitions needed to compile libpqxx into a library
#include "pqxx/config-internal-compiler.h"

// Library-private configuration related to libpq version
#include "pqxx/config-internal-libpq.h"

#ifdef _WIN32

#ifdef PQXX_SHARED
#undef  PQXX_LIBEXPORT
#define PQXX_LIBEXPORT	__declspec(dllexport)
// TODO: Does Windows have a way to "unexport" a symbol in an exported class?
#define PQXX_PRIVATE	__declspec()
#endif	// PQXX_SHARED

#ifdef _MSC_VER
#pragma warning (disable: 4251 4275 4273)
#pragma warning (disable: 4258) // Complains that for-scope usage is correct.
#pragma warning (disable: 4290)
#pragma warning (disable: 4351)
#pragma warning (disable: 4355)
#pragma warning (disable: 4786)
#pragma warning (disable: 4800)	// Performance warning for boolean conversions.
#pragma warning (disable: 4996) // Complains that strncpy() "may" be unsafe.
#endif

#elif defined(__GNUC__) && defined(PQXX_HAVE_GCC_VISIBILITY)	// !_WIN32

#define PQXX_LIBEXPORT __attribute__ ((visibility("default")))
#define PQXX_PRIVATE __attribute__ ((visibility("hidden")))

#endif	// __GNUC__ && PQXX_HAVE_GCC_VISIBILITY


#include "pqxx/compiler-public.hxx"

#include <cstddef>

#ifdef PQXX_HAVE_LIMITS
#include <limits>
#else // PQXX_HAVE_LIMITS
#include <climits>
namespace PGSTD
{
/// Work around lacking "limits" header
template<typename T> struct numeric_limits
{
  static T max() throw ();
  static T min() throw ();
};
template<> inline long numeric_limits<long>::max() throw () {return LONG_MAX;}
template<> inline long numeric_limits<long>::min() throw () {return LONG_MIN;}
}
#endif // PQXX_HAVE_LIMITS
#endif
