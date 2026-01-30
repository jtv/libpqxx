/* Compiler settings for compiling libpqxx headers, and workarounds for all.
 *
 * Include this before including any other libpqxx headers from within libpqxx.
 * And to balance it out, also include header-post.hxx at the end of the batch
 * of headers.
 *
 * The public libpqxx headers (e.g. `<pqxx/connection>`) include this already;
 * there's no need to do this from within an application.
 *
 * Include this file at the highest aggregation level possible to avoid nesting
 * and to keep things simple.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */

#if __has_include(<version>)
#  include <version>
#endif

// NO GUARD HERE! This part should be included every time this file is.
#if defined(_MSC_VER)

// Save compiler's warning state, and set warning level 4 for maximum
// sensitivity to warnings.
#  pragma warning(push, 4)

// Visual C++ generates some entirely unreasonable warnings.  Disable them.
#  pragma warning(disable : 4061 4251 4275 4275 4511 4512 4514 4623 4625)
#  pragma warning(disable : 4626 4702 4820 4868 5026 5027 5031 5045 6294)

#endif // _MSC_VER


#if defined(PQXX_HEADER_PRE)
#  error "Avoid nesting #include of pqxx/internal/header-pre.hxx."
#endif

#define PQXX_HEADER_PRE


// Workarounds & definitions that need to be included even in library's headers
#include "pqxx/config-compiler.h"

// MSVC has a nonstandard definition of __cplusplus.
#if defined(_MSC_VER)
#  define PQXX_CPLUSPLUS _MSVC_LANG
#else
#  define PQXX_CPLUSPLUS __cplusplus
#endif

#if __has_cpp_attribute(gnu::pure)
/// Declare function "pure": no side effects, only reads globals and its args.
/** Be careful with exceptions.  The compiler may elide calls, which may stop
 * an exception from happening; or reorder them, moving a call outside of a
 * `try` block that was meant to catch the exception.
 */
#  define PQXX_PURE [[gnu::pure]]
#else
#  define PQXX_PURE /* pure */
#endif


#if __has_cpp_attribute(gnu::cold)
/// Tell the compiler to optimise a function for size, not speed.
#  define PQXX_COLD [[gnu::cold]]
#else
#  define PQXX_COLD /* cold */
#endif


#if __has_cpp_attribute(gnu::always_inline)
/// Never generate an out-of-line version of this inline function.
#  define PQXX_INLINE_ONLY [[gnu::always_inline]]
#elif __has_cpp_attribute(msvc::forceinline)
/// Never generate an out-of-line version of this inline function.
#  define PQXX_INLINE_ONLY [[msvc::forceinline]]
#else
#  define PQXX_INLINE_ONLY /* always inline */
#endif


/// Don't generate out-of-line version of inline function for coverage runs.
/** This helps avoid a lot of false positives with gcov.  The out-of-line
 * function never gets executed, and so its code is all marked as not covered
 * by the test suite.  The code may get executed, but all inline.
 *
 * We're only defining this for gcc, since that's the compiler we use for test
 * coverage analysis.
 */
#if defined(PQXX_COVERAGE) && __has_cpp_attribute(gnu::always_inline)
#  define PQXX_INLINE_COV [[gnu::always_inline]]
#else
#  define PQXX_INLINE_COV /* inline-only on coverage runs */
#endif


#if __has_cpp_attribute(gnu::returns_nonnull)
/// For functions returning a pointer: the pointer is never null.
#  define PQXX_RETURNS_NONNULL [[gnu::returns_nonnull]]
#else
#  define PQXX_RETURNS_NONNULL /* returns nonnull */
#endif


#if __has_cpp_attribute(gnu::null_terminated_string_arg)
/// Function argument n (counted starting from 1) is a zero-terminated string.
#  define PQXX_ZARG(n) [[gnu::null_terminated_string_arg((n))]]
#else
#  define PQXX_ZARG(n) /* null-terminated string arg */
#endif


// Not all gcc versions we're seeing support the argument-less version.
#if defined(PQXX_HAVE_ZARGS)
/// This function's C-style string arguments are zero-terminated.
#  define PQXX_ZARGS [[gnu::null_terminated_string_arg]]
#else
#  define PQXX_ZARGS /* null-terminated string args */
#endif


// Workarounds for Windows
#ifdef _WIN32

/* For now, export DLL symbols if _DLL is defined.  This is done automatically
 * by the compiler when linking to the dynamic version of the runtime library,
 * according to "gzh"
 */
#  if defined(PQXX_SHARED) && !defined(PQXX_LIBEXPORT)
#    define PQXX_LIBEXPORT __declspec(dllimport)
#  endif // PQXX_SHARED && !PQXX_LIBEXPORT


// Workarounds for Microsoft Visual C++
#  ifdef _MSC_VER

// Suppress vtables on abstract classes.
#    define PQXX_NOVTABLE __declspec(novtable)

// Automatically link with the appropriate libpq (static or dynamic, debug or
// release).  The default is to use the release DLL.  Define PQXX_PQ_STATIC to
// link to a static version of libpq, and _DEBUG to link to a debug version.
// The two may be combined.
#    if defined(PQXX_AUTOLINK)
#      if defined(PQXX_PQ_STATIC)
#        ifdef _DEBUG
#          pragma comment(lib, "libpqd")
#        else
#          pragma comment(lib, "libpq")
#        endif
#      else
#        ifdef _DEBUG
#          pragma comment(lib, "libpqddll")
#        else
#          pragma comment(lib, "libpqdll")
#        endif
#      endif
#    endif

// If we're not compiling libpqxx itself, automatically link with the
// appropriate libpqxx library.  To link with the libpqxx DLL, define
// PQXX_SHARED; the default is to link with the static library.  A static link
// is the recommended practice.
//
// The preprocessor macro PQXX_INTERNAL is used to detect whether we
// are compiling the libpqxx library itself.  When you compile the library
// yourself using your own project file, make sure to include this macro.
#    if defined(PQXX_AUTOLINK) && !defined(PQXX_INTERNAL)
#      ifdef PQXX_SHARED
#        ifdef _DEBUG
#          pragma comment(lib, "libpqxxD")
#        else
#          pragma comment(lib, "libpqxx")
#        endif
#      else // !PQXX_SHARED
#        ifdef _DEBUG
#          pragma comment(lib, "libpqxx_staticD")
#        else
#          pragma comment(lib, "libpqxx_static")
#        endif
#      endif
#    endif

#  endif // _MSC_VER

#elif defined(PQXX_HAVE_GCC_VISIBILITY) // !_WIN32

#  define PQXX_LIBEXPORT [[gnu::visibility("default")]]
#  define PQXX_PRIVATE [[gnu::visibility("hidden")]]

#endif // PQXX_HAVE_GCC_VISIBILITY


#ifndef PQXX_LIBEXPORT
#  define PQXX_LIBEXPORT /* libexport */
#endif

#ifndef PQXX_PRIVATE
#  define PQXX_PRIVATE /* private */
#endif

#ifndef PQXX_NOVTABLE
#  define PQXX_NOVTABLE /* novtable */
#endif

// C++23: Assume support.
#if defined(PQXX_HAVE_ASSUME)
#  define PQXX_ASSUME(condition) [[assume(condition)]]
#else
#  define PQXX_ASSUME(condition) while (false)
#endif
