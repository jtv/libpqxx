/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/compiler.h
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds for compiling libpqxx itself.
 *      DO NOT INCLUDE THIS FILE when building client programs.
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_COMPILER_H
#define PQXX_COMPILER_H

// Workarounds & definitions needed to compile libpqxx into a library
#include "pqxx/config.h"
#include "pqxx/libcompiler.h"


#ifdef HAVE_LIMITS
#include <limits>
#else // HAVE_LIMITS
#include <climits>
namespace PGSTD
{
/// Work around lacking "limits" header
template<typename T> struct numeric_limits
{
  static T max() throw ();
  static T min() throw ();
};

/// Work around lacking std::max()
template<> inline long numeric_limits<long>::max() throw () {return LONG_MAX;}
/// Work around lacking std::min()
template<> inline long numeric_limits<long>::min() throw () {return LONG_MIN;}
}
#endif // HAVE_LIMITS


#ifndef HAVE_ABS_LONG
// For compilers that feel abs(long) is ambiguous
long abs(long n) { return (n >= 0) ? n : -n; }
#endif // HAVE_ABS_LONG

#ifdef _WIN32
#ifdef LIBPQXXDLL_EXPORTS
#define PQXX_LIBEXPORT __declspec(dllexport)
#endif	// LIBPQXXDLL_EXPORTS
#endif	// _WIN32

#endif

