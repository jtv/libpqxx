/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/compiler.h
 *
 *   DESCRIPTION
 *      Compiler deficiency workarounds
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_COMPILER_H
#define PQXX_COMPILER_H

#include "pqxx/config.h"

#ifdef BROKEN_ITERATOR
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
#endif // BROKEN_ITERATOR

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

// Used for Windows DLL
#ifndef PQXX_LIBEXPORT
#define PQXX_LIBEXPORT
#endif

#endif

