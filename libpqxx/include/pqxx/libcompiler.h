/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/libcompiler.h
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds for libpqxx clients
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
#include "pqxx/libconfig.h"

#ifdef PQXX_BROKEN_ITERATOR
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

#ifndef HAVE_CHAR_TRAITS
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

  static int_type eof() { return -1; }
};
}
#endif

// Workarounds for Windows
#ifdef _WIN32

#ifdef _MSC_VER
#if _MSC_VER < 1300
#error If you're using Visual C++, you'll need at least version 7 (VC.NET)
#endif	// _MSC_VER < 1300

// Workarounds for Visual C++.NET
#define PQXX_WORKAROUND_VC7
#define PQXX_NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION
#define PQXX_TYPENAME
#pragma warning (disable: 4290)
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

