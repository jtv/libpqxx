/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/compiler.h
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds
 *
 * Copyright (c) 2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_COMPILER_H
#define PQXX_COMPILER_H

#include "pqxx/config.h"

#ifdef BROKEN_ITERATOR
namespace PGSTD
{
/// Deal with lacking iterator template definition in <iterator>
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
#endif // BROKEN_ITERATOR


#ifdef HAVE_LIMITS
#include <limits>
#else // HAVE_LIMITS
#include <climits>
namespace PGSTD
{
/// Deal with lacking <limits>
template<typename T> struct numeric_limits
{
  static T max() throw ();
  static T min() throw ();
};

template<> inline long numeric_limits<long>::max() throw () {return LONG_MAX;}
template<> inline long numeric_limits<long>::min() throw () {return LONG_MIN;}
}
#endif // HAVE_LIMITS


// Used for Windows DLL
#define PQXX_LIBEXPORT


#ifdef _MSC_VER
// Microsoft Visual C++ likes to complain about long debug symbols, which
// are a fact of life in modern C++.  Silence the warning.
#pragma warning (disable: 4786)

// Compiler doesn't know if it is import or export
#pragma warning (disable: 4251 4275)

// Link to libpq
#pragma comment(lib, "libpqdll")

#undef PQXX_LIBEXPORT
#ifndef LIBPQXXDLL_EXPORTS	// Defined in Makefile
#define PQXX_LIBEXPORT __declspec(dllimport)
#else
#define PQXX_LIBEXPORT __declspec(dllexport)
#endif	// LIBPQXXDLL_EXPORTS

#endif	// _MSC_VER


#endif

