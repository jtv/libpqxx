// This section has been moved from compiler.h. The use of autoconf and moved
// compiler specific information to the config.h file.  As Win32 doesn't have
// and autoconf tool a hand tooled config.h files has been created.

// Microsoft Visual C++ likes to complain about long debug symbols, which
// are a fact of life in modern C++.  Silence the warning.
#ifndef _WIN32
#error This file is only for use with windows compilers. Currently only vc7 is supported.
#endif

#if _MSC_VER < 1300
#error This has not been tested for compilers before visual c++ 7 ie .NET
#endif

#pragma warning (disable: 4786)

// Compiler doesn't know if it is import or export
#pragma warning (disable: 4251 4275 4273)

// Link to libpq
// We are going to go ahead and tell the linker we want the libpq linked in
// I have gone ahead and compiled it as a dll and it was named libpqdll. 
// You may need to change this to fit your personal needs. Compiling libpq 
// as a dll or static library are a seperate issue.
#pragma comment(lib, "libpqdll")

// We need the classes exported 
#ifndef LIBPQXXDLL_EXPORTS	// Defined in Makefile
  #define PQXX_LIBEXPORT __declspec(dllimport)
#else
  #define PQXX_LIBEXPORT __declspec(dllexport)
#endif	// LIBPQXXDLL_EXPORTS

/* Does the standard namespace exist? */
#define PGSTD std

/* Define if <iterator> lacks an iterator template definition */
//#define BROKEN_ITERATOR 1

/* Use postgres SQL? */
#define DIALECT_POSTGRESQL 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <iterator> header file. */
#define HAVE_ITERATOR 1

/* Define to 1 if you have the `pq' library (-lpq). */
#define HAVE_LIBPQ 1

/* Define to 1 if you have the <limits> header file. */
#define HAVE_LIMITS 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if the system has the type `ptrdiff_t'. */
#define HAVE_PTRDIFF_T 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Name of package */
#define PACKAGE "libpqxx"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "Jeroen T. Vermeulen <jtv@xs4all.nl>"

/* Define to the full name of this package. */
#define PACKAGE_NAME "libpqxx"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "libpqxx 1.0.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "libpqxx"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.0.0"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.0.0"

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define as `__inline' if that's what the C compiler calls it, or to nothing
   if it is not supported. */
/* #undef inline */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */
