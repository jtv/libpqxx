# Generate configure script for libpqxx.  This needs the autoconf archive
# package installed.  (The configure script itself does not require it though.)

AC_PREREQ([2.71])
AC_INIT([libpqxx],[m4_esyscmd_s([./tools/extract_version])],[Jeroen T. Vermeulen])
AC_LANG(C++)

AC_CONFIG_SRCDIR([src/connection.cxx])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config/m4])
AM_INIT_AUTOMAKE([subdir-objects])

PQXX_ABI=m4_esyscmd_s([./tools/extract_version --abi])
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


# It's tempting to use Autoconf Archive's AX_CXX_COMPILE_STDCXX_17 for this,
# but it's 2022 and the C++20 equivalent isn't quite ready for use.
# Seems simpler and more reliable for the user to arrange for the desired
# language versions by setting the appropriate option for their compiler.
AC_MSG_CHECKING([for sufficient C++ language/library level])
sufficient_cxx=yes
AC_COMPILE_IFELSE(
	[AC_LANG_SOURCE([
	#if __cplusplus < 201611L
	#error "Need C++17 or better."
	#endif
	])],
	sufficient_cxx=yes,
	sufficient_cxx=no)
AC_MSG_RESULT($sufficient_cxx)
if test "$sufficient_cxx" != "yes"
then
	AC_MSG_ERROR([This libpqxx version needs at least C++17.])
fi


# Let's try to get the compiler to be helpful.
#
# (Omit options -Weffc++ and -Wabi because they currently yield too many
# warnings in gcc's standard headers; omit -Wunreachable-code because it isn't
# always right)
if test "$GCC" = "yes"
then
	# In maintainer mode, enable all the warning options we can.
	if test "$enable_maintainer_mode" = "yes"
	then
		# "Eternal" (FLW) g++ options.  These have been around for
		# ages, and both g++ and clang++ support them.  Don't bother
		# checking for support; just add them to the compiler options.
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
		# The -fanalyzer one is a macro option for many of the others.
		add_compiler_opts_if_ok \
			-fanalyzer \
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
			-fnothrow-opt \
			-Wattribute-alias=2 \
			-Wlogical-op \
			-Wmismatched-tags \
			-Wredundant-tags \
			-Wrestrict \
			-Wstringop-overflow \
			-Warray-bounds=2 \
			-Wduplicated-branches \
			-Wduplicated-cond \
			-Wsuggest-attribute=noreturn \
			-Wsuggest-override \
			-Wtrampolines
	fi

	# In "audit," enable all runtime checks we can.
	if test "$enable_audit" = "yes"
	then
		add_compiler_opts_if_ok \
			-D_FORTIFY_SOURCE=2 \
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

        # We have nothing to tell us where the libpq headers are.  That's fine
        # if the compiler can find it, but if not, fail here.
        AC_CHECK_HEADER(
	        [libpq-fe.h],
	        [],
	[AC_MSG_ERROR([
Can't find the main PostgreSQL client header, libpq-fe.h.  Make sure that it
is installed, and either use the --with-postgres-include option or install
pkg-config.
])])
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


# TODO: PQresultMemorySize()


AC_MSG_CHECKING([whether we need a link option for <filesystem> support])
LIBS_SAVE="$LIBS"
found_fslib=no
for l in '' '-lstdc++fs' '-lc++fs'
do
	if test "$found_fslib" != "yes"
	then
		LIBS="$LIBS $l"
		AC_LINK_IFELSE(
			[read_test(no_need_fslib.cxx)],
			[found_fslib=yes],
			[LIBS="$LIBS_SAVE"])
		if test "$found_fslib" = "yes"
		then
			result_msg="$l"
			# (And keep our current value of $LIBS.)
		fi
	fi
done

if test "$found_fslib" != "yes"
then
	AC_MSG_ERROR([
There seems to be <filesystem> support, but I could not figure out now to make
it work.  You'll have to set your own build options for this.
	])
fi
AC_MSG_RESULT($result_msg)


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
	Makefile config/Makefile doc/Makefile src/Makefile test/Makefile
	tools/Makefile include/Makefile include/pqxx/Makefile libpqxx.pc])


AC_CONFIG_COMMANDS([configitems],
	["${srcdir}/tools/splitconfig.py" "${srcdir}"])

AC_CONFIG_FILES([compile_flags])
AC_OUTPUT
