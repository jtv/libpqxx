# Generate configure script for libpqxx.  This needs the autoconf archive
# package installed.  (The configure script itself does not require it though.)

AC_PREREQ([2.71])
AC_INIT([libpqxx],[m4_esyscmd_s([./tools/extract_version.sh])],
	[Jeroen T. Vermeulen])

# Suppress checks for standard C headers.  One, we're not doing C here; two,
# if you don't have these, chances are your system is in no shape to build
# libpqxx; and three, we require C++20.  It's a bit pointless to guard against
# such specific shortcomings of 20th-century compilers.
m4_define([AC_INCLUDES_DEFAULT], [])

AC_LANG(C++)
AX_CXX_COMPILE_STDCXX([20])

AC_CONFIG_SRCDIR([src/connection.cxx])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config/m4])
AM_INIT_AUTOMAKE([subdir-objects])

PQXX_ABI=m4_esyscmd_s([./tools/extract_version.sh --abi])
AC_SUBST(PQXXVERSION, $PACKAGE_VERSION)
AC_SUBST(PQXX_ABI)

AC_CONFIG_HEADERS([include/pqxx/config.h])

# Default prefix for installs.
AC_PREFIX_DEFAULT(/usr/local)


# Read test programme from config-test.
AC_DEFUN([read_test], [AC_LANG_SOURCE(
	esyscmd(tools/m4esc.py --input=config-tests/$1))])


# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_DISABLE_SHARED
LT_INIT
AC_PROG_MAKE_SET
AC_PATH_PROG([MKDIR], [mkdir])

AC_ARG_ENABLE(
	documentation,
	[AS_HELP_STRING([--enable-documentation], [Generate documentation])],
	[enable_documentation="${enableval}"])
AM_CONDITIONAL(
	[BUILD_DOCS],
	[test "$enable_documentation" = "yes"])

AM_MAINTAINER_MODE

# See if we want stricter compiler warnings.
AC_MSG_CHECKING([maintainer mode])
AC_ARG_ENABLE(maintainer-mode)
AC_MSG_RESULT(${enable_maintainer_mode})

# See if we want runtime debug checking.
AC_MSG_CHECKING([audit])
AC_ARG_ENABLE(audit)
AC_MSG_RESULT(${enable_audit})

# See if we want "suggestions," such as "this class could be final."
# (The suggestions are often useful, but can also easily be wrong.)
AC_MSG_CHECKING([suggest])
AC_ARG_ENABLE(suggest)
AC_MSG_RESULT(${enable_suggest})


AC_ARG_ENABLE(shared)
AS_IF(
    [test "${shared}" = "yes" ],
    [CPPFLAGS="$CPPFLAGS -DPQXX_SHARED"])


# Add options to compiler command line, if compiler accepts them.
add_compiler_opts_if_ok() {
	for option in $*
	do
		ACO_SAVE_CXXFLAGS="$CXXFLAGS"
		CXXFLAGS="$CXXFLAGS $option"
		AC_MSG_CHECKING([whether $CXX accepts $option])
		AC_COMPILE_IFELSE(
			[AC_LANG_PROGRAM([], [])],
			has_option=yes,
			has_option=no)
		AC_MSG_RESULT($has_option)
		AS_IF(
		    [test "$has_option" = "no" ],
		    [CXXFLAGS="$ACO_SAVE_CXXFLAGS"])
	done
}


# Add options to compiler command line, unconditionally.
add_compiler_opts() {
	CXXFLAGS="$CXXFLAGS $*"
}


# Let's try to get the compiler to be helpful.
#
# (Omit options -Weffc++ and -Wabi because they currently yield too many
# warnings in gcc's standard headers.)
if test "$GCC" = "yes"
then
	# In maintainer mode, enable all the warning options we can.
	if test "$enable_maintainer_mode" = "yes"
	then
		# "Eternal" (FLW) g++ options.  These have been around for
		# ages, and both g++ and clang++ support them.  Don't bother
		# checking for support; just add them to the compiler options.
		#
		# We'd love to add -D_GLIBCXX_CONCEPT_CHECKS in 8.0 here, but
		# it complains that the "reference" type in result iterators is
		# not an actual reference.
		add_compiler_opts \
			-fstrict-enums \
			-Werror \
			-Wall \
			-pedantic \
			-Wcast-align  \
			-Wcast-qual  \
			-Wconversion \
			-Wctor-dtor-privacy \
			-Wendif-labels \
			-Wextra \
			-Wextra-semi \
			-Wfloat-equal \
			-Wformat=2 \
			-Wformat-security \
			-Wmissing-include-dirs \
			-Wno-div-by-zero \
			-Wnon-virtual-dtor \
			-Wold-style-cast \
			-Woverlength-strings \
			-Woverloaded-virtual \
			-Wpointer-arith  \
			-Wredundant-decls \
			-Wshadow \
			-Wsign-promo \
			-Wundef \
			-Wunused \
			-Wwrite-strings \
			-Wzero-as-null-pointer-constant \

		# "Iffy" g++ options.  Some reasonably current g++-like
		# compilers may not support these.
		#
		# The -fanalyzer one is a macro option for many of the others,
		# but we can't use it because as of gcc 13.2.0 we still see it
		# generating errors for the standard library.
		add_compiler_opts_if_ok \
			-fnothrow-opt \
			-Wanalyzer-double-fclose \
			-Wanalyzer-double-free \
			-Wanalyzer-exposure-through-output-file \
			-Wanalyzer-file-leak \
			-Wanalyzer-free-of-non-heap \
			-Wanalyzer-malloc-leak \
			-Wanalyzer-possible-null-dereference \
			-Wanalyzer-mismatching-deallocation \
			-Wanalyzer-null-dereference \
			-Wanalyzer-null-argument \
			-Wanalyzer-possible-null-argument \
			-Wanalyzer-shift-count-negative \
			-Wanalyzer-overflow \
			-Wanalyzer-stale-setjmp-buffer \
			-Wanalyzer-tainted-array-index \
			-Wanalyzer-unsafe-call-within-signal-handler \
			-Wanalyzer-use-after-free \
			-Wanalyzer-use-of-pointer-in-stale-stack-frame \
			-Wanalyzer-use-of-uninitialized-value \
			-Wanalyzer-write-to-const \
			-Wanalyzer-write-to-string-literal \
			-Warray-bounds=2 \
			-Wattribute-alias=2 \
			-Wdangling-gsl \
			-Wdangling-reference \
			-Wdeprecated-literal-operator \
			-Wduplicated-branches \
			-Wduplicated-cond \
			-Wlogical-op \
			-Wmismatched-tags \
			-Wredundant-tags \
			-Wrestrict \
			-Wstringop-overflow \
			-Wsuggest-attribute=noreturn \
			-Wsuggest-override \
			-Wtrampolines \
			-Wunreachable-code
	fi

	# In "audit," enable all runtime checks we can.
	if test "$enable_audit" = "yes"
	then
		# These macros enable extra runtime checking in various
		# standard library implementations.
		add_compiler_opts \
			-D_FORTIFY_SOURCE=2 \
			-D_GLIBCXX_ASSERTIONS \
			-D_GLIBCXX_DEBUG \
			-D_GLIBCXX_DEBUG_PEDANTIC \
			-D_GLIBCXX_SANITIZE_VECTOR \
			-D_LIBCPP_DEBUG=1 \
			-D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG
		# Run-time sanitizers and such.  These can differ from one
		# compiler (or version) to the next.
		add_compiler_opts_if_ok \
			-fno-sanitize-recover=all \
			-fsanitize=address \
			-fsanitize-address-use-after-scope \
			-fsanitize=alignment \
			-fsanitize=bool \
			-fsanitize=bounds \
			-fsanitize=bounds-strict \
			-fsanitize=builtin \
			-fsanitize=enum \
			-fsanitize=float-cast-overflow \
			-fsanitize=float-divide-by-zero \
			-fsanitize=integer-divide-by-zero \
			-fsanitize=leak \
			-fsanitize=nonnull-attribute \
			-fsanitize=null \
			-fsanitize=object-size \
			-fsanitize=pointer-compare \
			-fsanitize=pointer-overflow \
			-fsanitize=pointer-subtract \
			-fsanitize=return \
			-fsanitize=returns-nonnull-attribute \
			-fsanitize=shift \
			-fsanitize=shift-base \
			-fsanitize=shift-exponent \
			-fsanitize=signed-integer-overflow \
			-fsanitize=undefined \
			-fsanitize=unreachable \
			-fsanitize=vla-bound \
			-fsanitize=vptr \
			-fstack-protector-all
	fi

	# In "suggest" mode, enable a bunch of code suggestions.
	if test "$enable_suggest" = "yes"
	then
		add_compiler_opts_if_ok \
			-Wsuggest-attribute=cold \
			-Wsuggest-attribute=const \
			-Wsuggest-attribute=malloc \
			-Wsuggest-attribute=pure \
			-Wsuggest-final-types \
			-Wsuggest-final-methods
	fi
fi # End of gcc-specific part.


m4_include([pqxx_cxx_feature_checks.ac])


# One of the generated checks sets this variable.
if test "$PQXX_HAVE_GCC_VISIBILITY" = "yes"
then
    # Make internal definitions accessible only to the library itself.
    # Only definitions marked PQXX_LIBEXPORT will be accessible.
    add_compiler_opts -fvisibility=hidden
    add_compiler_opts -fvisibility-inlines-hidden
fi


# One of the generated checks sets this variable.
if test "$PQXX_HAVE_POLL" != "yes"
then
# No poll(); we'll fall back to select().

# Some systems keep select() in a separate library which is not linked by
# default.  See if we need one of those.
socklibok=no
AC_SEARCH_LIBS(select, socket nsl ws2_32 wsock32 winsock, [socklibok=yes])

# Microsoft proprietary libraries do not work with code that is generated with
# autoconf's SEARCH_LIBS macro, so we need to check manually and just use the
# first socket library available.
# We only do this if select() is not available by other means, to avoid picking
# up an unnecessary Windows compatibility library on a non-Windows system.
for l in ws2_32 wsock32 winsock
do
	if test "${socklibok}" != "yes"
	then
		AC_CHECK_LIB($l,main,LIBS="$LIBS -l$l";[socklibok=yes])
	fi
done

if test "${socklibok}" != "yes"
then
	AC_MSG_ERROR([
Could not figure out how to link a simple sockets-based program.  Please read
the config.log file for more clues as to why this failed.
])
fi

fi # No poll()


# Check for std::stacktrace support.
#
# We need to link this test because some compilers require an extra library to
# support std::stacktrace.
AC_MSG_CHECKING([for std::stacktrace support])
have_stacktrace=yes
AC_LINK_IFELSE(
    [read_test(stacktrace_support.cxx)],
    AC_DEFINE([PQXX_HAVE_STACKTRACE], 1, [Define if std::stacktrace works.]),
    have_stacktrace=no)
AC_MSG_RESULT($have_stacktrace)


# Find PostgreSQL includes and libraries
AC_PATH_PROG([PKG_CONFIG], [pkg-config])
AC_PATH_PROGS(PG_CONFIG, pg_config)

AC_ARG_WITH(
    [postgres-include],
    [AS_HELP_STRING(
        [--with-postgres-include=DIR],
        [Use PostgreSQL includes from DIR.  Defaults to querying pg_config or pkg-config, whichever is available.])],
    AS_IF(
        [test "x$with_postgres_include" = "xyes"],
        [with_postgres_include=""]))

if test -n "$with_postgres_include"
then
    POSTGRES_INCLUDE="-I$with_postgres_include"
else
    if test -x "$PKG_CONFIG" || test -x "$PG_CONFIG"
    then
        # We should prefer pkg-config over pg_config, but there seems to be a
        # problem in pkg-config 1.6.3.  Until that's been resolved (#291), go
        # with pg_config if we can.
        if test -x "$PG_CONFIG"
        then
            # From pg_config we can either get the C compiler options used to
            # compile postgres, which isn't quite what we want; or we can get
            # the headers directory, without the full option.  That's something
            # we can work with.  The compiler must support the "-I" option for
            # that, but both scripts assume that anyway.
            POSTGRES_INCLUDE="-I$($PG_CONFIG --includedir)"
        else
            # From pkg-config we can get the compiler options to extend the
            # include path.  We use that.
            POSTGRES_INCLUDE=$($PKG_CONFIG libpq --cflags-only-I)
        fi
        AC_MSG_NOTICE([finding PostgreSQL headers using $POSTGRES_INCLUDE])
    else
        POSTGRES_INCLUDE=""
    fi
fi
AC_SUBST(POSTGRES_INCLUDE)

# Add the compiler option so we can compile configure tests which rely on the
# libpq headers.
CPPFLAGS="$CPPFLAGS $POSTGRES_INCLUDE"


AC_ARG_WITH(
    [postgres-lib],
    [AS_HELP_STRING(
        [--with-postgres-lib=DIR],
        [Use PostgreSQL libraries from DIR.  Defaults to querying pg_config.])],
    AS_IF(
        [test "x$with_postgres_lib" = "xyes"],
        [with_postgres_lib=""]))

# If no --with-postgres-lib was given, and we have pkg-config, use that.
AS_IF(
    [test -z "$with_postgres_lib" -a -x "$PKG_CONFIG"],
    [with_postgres_lib=$($PKG_CONFIG libpq --libs-only-L | sed 's/^-L//')])

# pg_config is deprecated, but for some users it may still provide the only
# right answer.  For instance, `pkg-config` may not know where `libpq` is
# installed.
AS_IF(
    [test -z "$with_postgres_lib" -a -x "$PG_CONFIG"],
    [with_postgres_lib=$($PG_CONFIG --libdir)])

AS_IF(
    [test -n "$with_postgres_lib"],
    [AC_MSG_NOTICE([using PostgreSQL libraries at $with_postgres_lib])],
    [AC_MSG_NOTICE([using PostgreSQL libraries in default location])])

AC_SUBST(with_postgres_lib)


AC_CHECK_HEADER(
	[libpq-fe.h],
	[],
	[AC_MSG_ERROR([
Can't find the main PostgreSQL client header, libpq-fe.h.  Are you sure the
libpq headers are installed correctly, and that we've got the right path?
])])


AC_MSG_CHECKING([for ability to compile source files using libpq])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include<libpq-fe.h>]],
		[[PQexec(nullptr,"")]]
	)],
	[],
	[AC_MSG_ERROR([
Could not compile a call to a basic libpq function.  There must be something
seriously wrong with the headers that "pg_config --includedir" or "pkg-config
libpq --cflags" pointed to; the contents of config.log may give you a clue
about the nature of the failure.
Source including the libpq header libpq-fe.h can be compiled, but a call to the
most basic libpq function PQexec() failed to compile successfully.  This is the
litmus test for a working libpq.
])])
AC_MSG_RESULT(yes)


if test "x${with_postgres_lib}" = "x"; then
    with_postgres_libpath=""
else
    with_postgres_libpath="-L${with_postgres_lib}"
fi
LDFLAGS="$LDFLAGS ${with_postgres_libpath}"

AC_CHECK_LIB(
	[pq],
	[PQexec],
	[],
	[AC_MSG_ERROR([
Did not find the PQexec() function in libpq.  This is the litmus test for a
working libpq installation.

A source file using the PQexec() function did compile without problems, and the
libpq library is available for linking, but for some reason a call to PQexec()
failed to link properly to the libpq library.  This may be because the libpq
library file is damaged, or in some incorrect format, or if your libpq is much
more recent than libpqxx version $PQXX_ABI, perhaps libpq has undergone a
radical ABI change.

The last parts of config.log may give you a clue as to what really went wrong,
but be warned that this is no easy reading.  Look for the last error message
occurring in the file.
])],
	${with_postgres_libpath})


# Remove redundant occurrances of -lpq
LIBS=[$(echo "$LIBS" | sed -e 's/-lpq * -lpq\>/-lpq/g')]


AC_MSG_CHECKING([that type of libpq's Oid is as expected])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[
			#include<libpq-fe.h>
			#include"${srcdir}/include/pqxx/internal/libpq-forward.hxx"
			extern void f(pqxx::oid&);
		],
		[Oid o;f(o)],
	)],
	[],
	[AC_MSG_ERROR([
The Oid typedef in libpq has changed.  Please notify the libpqxx authors of the
change!
])])
AC_MSG_RESULT(yes)


AC_PROG_MAKE_SET

AC_CONFIG_FILES([
	Makefile config/Makefile doc/Makefile examples/Makefile src/Makefile
	test/Makefile tools/Makefile include/Makefile include/pqxx/Makefile
	libpqxx.pc])


# We've let autoconf generate a config.h.  But there's a lot of stuff in there
# that we don't need, and that we absolutely do not want.  We'd like to avoid
# polluting the application's namespace with macros that aren't clearly marked
# as libpqxx-related.
#
# So we take just the ones with the "PQXX" marker and write them into a
# header just for compiler-related configuration.
AC_CONFIG_COMMANDS([configitems],
	[grep PQXX include/pqxx/config.h >include/pqxx/config-compiler.h])

AC_CONFIG_FILES([compile_flags])
AC_OUTPUT
