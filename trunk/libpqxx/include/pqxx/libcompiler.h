/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/libcompiler.h
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds for libpqxx clients
 *
 * Copyright (c) 2002-2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_LIBCOMPILER_H
#define PQXX_LIBCOMPILER_H

// Workarounds & definitions that need to be included even in library's headers
#include "pqxx/config-public-compiler.h"


#ifdef PQXX_BROKEN_ITERATOR
#include <cstddef>
#include <cstdlib>
/// Alias for the std namespace to accomodate nonstandard C++ implementations
/** The PGSTD name will almost always be defined to mean std.  The exception are
 * third-party C++ standard library implementations that use a different 
 * namespace to avoid conflicts with the standard library that came with the
 * compiler.
 *
 * Some definitions that appear missing in the standard library of the host
 * system may be added to get libpqxx working.
 */
namespace PGSTD
{
/// Work around lacking iterator template definition in <iterator>
template<typename Cat, 
         typename T, 
	 typename Dist, 
	 typename Ptr=T*,
	 typename Ref=T&> struct iterator
{
  typedef Cat iterator_category;
  typedef T value_type;
  typedef Dist difference_type;
  typedef Ptr pointer;
  typedef Ref reference;
};
}
#else
#include <iterator>
#endif // PQXX_BROKEN_ITERATOR

#ifndef PQXX_HAVE_CHAR_TRAITS
#include <cstddef>
namespace PGSTD
{
/// Work around missing std::char_traits
template<typename CHAR> struct char_traits {};
/// Work around missing std::char_traits<char>
template<> struct char_traits<char>
{
  typedef int int_type;
  typedef size_t pos_type;
  typedef long off_type;
  typedef char char_type;

  static int_type eof() { return -1; }
};
/// Work around missing std::char_traits<unsigned char>
template<> struct char_traits<unsigned char>
{
  typedef int int_type;
  typedef size_t pos_type;
  typedef long off_type;
  typedef unsigned char char_type;

  static int_type eof() { return -1; }
};
}
#endif

// Workarounds for SUN Workshop 6
#if defined(__SUNPRO_CC)
#if __SUNPRO_CC_COMPAT < 5
#error "This compiler version is not capable of building libpqxx."
#endif	// __SUNPRO_CC_COMPAT < 5
#endif	// __SUNPRO_CC


// Workarounds for Compaq C++ for Alpha
#if defined(__DECCXX_VER)
#define __USE_STD_IOSTREAM
#endif	// __DECCXX_VER


// Workarounds for Windows
#ifdef _WIN32

// Workarounds for Microsoft Visual C++
#ifdef _MSC_VER

#if _MSC_VER < 1300
#error If you're using Visual C++, you'll need at least version 7 (VC.NET)
#elif _MSC_VER < 1310
// Workarounds for pre-2003 Visual C++.NET
#undef PQXX_HAVE_REVERSE_ITERATOR
#define PQXX_NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION
#define PQXX_TYPENAME
#endif	// _MSC_VER < 1310

#pragma warning (disable: 4290)
#pragma warning (disable: 4355)
#pragma warning (disable: 4786)
#pragma warning (disable: 4251 4275 4273)
#pragma comment(lib, "libpqdll")
#if !defined(PQXX_LIBEXPORT) && !defined(_LIB)
#define PQXX_LIBEXPORT __declspec(dllimport)
#endif	// PQXX_LIBEXPORT _LIB

// TODO: Restrict this to compiler versions known to be affected
/* Work around a particularly pernicious and deliberate bug in Visual C++:
 * min() and max() are defined as macros, which can have some very nasty
 * consequences.  This compiler bug can be switched off by defining NOMINMAX.
 *
 * We don't like making choices for the user and defining environmental macros
 * of our own accord, but in this case it's the only way to compile without
 * incurring a significant risk of bugs--and there doesn't appear to be any
 * downside.  One wonders why this compiler wart is being maintained at all,
 * since the introduction of inline functions back in the 20th century.
 */
#if defined(min) || defined(max)
#error "Oops: min() and/or max() are defined as preprocessor macros.\
  Define NOMINMAX macro before including any system headers!"
#endif
#define NOMINMAX

/// Apparently Visual C++.NET 2003 breaks on stdout/stderr output in destructors
/** Defining this macro will disable all error or warning messages whenever a
 * destructor of a libpqxx-defined class is being executed.  This may cause
 * important messages to be lost, but I'm told the code will crash without it.
 * Of course it's only a partial solution; the client code may still do bad
 * things from destructors and run into the same problem.
 *
 * If this workaround does solve the crashes, we may have to work out some
 * system of deferred messages that will remember the messages and re-issue them
 * after all known active destructors has finished.  But that could be
 * error-prone: what if memory ran out while trying to queue a message, for
 * instance?  The only solution may be for the vendor to fix the compiler.
 */
#define PQXX_QUIET_DESTRUCTORS

#endif	// _MSC_VER
#endif	// _WIN32

// Used for Windows DLL
#ifndef PQXX_LIBEXPORT
#define PQXX_LIBEXPORT
#endif

// Some compilers (well, VC) stumble over some required cases of "typename"
#ifndef PQXX_TYPENAME
#define PQXX_TYPENAME typename
#endif

#endif

