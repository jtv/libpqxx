/* Compiler settings for compiling libpqxx itself.
 *
 * Include this header in every source file that goes into the libpqxx library
 * binary, and nowhere else.
 *
 * To ensure this, include this file once, as the very first header, in each
 * compilation unit for the library.
 *
 * DO NOT INCLUDE THIS FILE when building client programs.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_COMPILER_INTERNAL
#define PQXX_H_COMPILER_INTERNAL


// Workarounds & definitions needed to compile libpqxx into a library
#include "pqxx/config-internal-compiler.h"

// TODO: Check for Visual Studio, not for Windows.
#ifdef _WIN32

#  ifdef PQXX_SHARED
// We're building libpqxx as a shared library.
#    undef PQXX_LIBEXPORT
#    define PQXX_LIBEXPORT __declspec(dllexport)
#    define PQXX_PRIVATE __declspec()
#  endif // PQXX_SHARED

#elif defined(__GNUC__) && defined(PQXX_HAVE_GCC_VISIBILITY) // !_WIN32

#  define PQXX_LIBEXPORT __attribute__((visibility("default")))
#  define PQXX_PRIVATE __attribute__((visibility("hidden")))

#endif // __GNUC__ && PQXX_HAVE_GCC_VISIBILITY

#include "pqxx/compiler-public.hxx"
#endif
