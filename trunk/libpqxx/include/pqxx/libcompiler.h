/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/libcompiler.h
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds for libpqxx clients
 *
 * Copyright (c) 2002-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


#ifndef PQXX_HAVE_PTRDIFF_T
typedef long ptrdiff_t;
#endif


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
  typedef ptrdiff_t off_type;
  typedef char char_type;

  static int_type eof() { return -1; }
};
/// Work around missing std::char_traits<unsigned char>
template<> struct char_traits<unsigned char>
{
  typedef int int_type;
  typedef size_t pos_type;
  typedef ptrdiff_t off_type;
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

#define PQXX_BROKEN_MEMBER_TEMPLATE_DEFAULT_ARG

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
#endif	// _MSC_VER < 1300

// Workarounds for Visual C++.NET (2003 version does seem to work)
#if _MSC_VER < 1310
#define PQXX_WORKAROUND_VC7
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

