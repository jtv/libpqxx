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
#ifndef PG_COMPILER_H
#define PG_COMPILER_H

// Allow for different std namespace (eg. 3rd party STL implementations)
#ifndef PGSTD
#define PGSTD std
#endif


// Deal with lacking iterator template definition in <iterator>
#ifdef LACK_ITERATOR
namespace PGSTD
{
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
#endif // LACK_ITERATOR



// Deal with lacking <limits>
#ifdef LACK_LIMITS
#include <climits>
namespace PGSTD
{
template<typename T> struct numeric_limits
{
  static T max() throw ();
  static T min() throw ();
};

template<> inline long numeric_limits<long>::max() throw () {return LONG_MAX;}
template<> inline long numeric_limits<long>::min() throw () {return LONG_MIN;}
}
#else // LACK_LIMITS
#include <limits>
#endif // LACK_LIMITS


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

