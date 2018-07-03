# Generate configure script for libpqxx.  This needs the autoconf archive
# package installed.  (The configure script itself does not require it though.)

AC_PREREQ(2.69)
AC_INIT(
	libpqxx,
	[m4_esyscmd_s([./tools/extract_version])],
	[Jeroen T. Vermeulen])
AC_LANG(C++)
# Require C++11 at minimum.
AX_CXX_COMPILE_STDCXX_11([noext])
AC_CONFIG_SRCDIR([src/connection.cxx])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config/m4])
AM_INIT_AUTOMAKE

PQXX_ABI=m4_esyscmd_s([./tools/extract_version --abi])
AC_SUBST(PQXXVERSION)
AC_SUBST(PQXX_ABI)

AC_CONFIG_HEADER([include/pqxx/config.h])

# Default prefix for installs.
AC_PREFIX_DEFAULT(/usr/local)

# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PATH_PROG([MKDIR], [mkdir])
AC_PATH_PROG([PKG_CONFIG], [pkg-config])

# Documentation.
AC_ARG_ENABLE(
	documentation,
	[AS_HELP_STRING([--enable-documentation], [Generate documentation])],
	[],
	[enable_documentation=auto])
AC_ARG_VAR([DOXYGEN],
	[Path to doxygen needed to build reference documentation])
AC_PATH_TOOL([DOXYGEN], [doxygen], [nodoxygen])
AC_ARG_VAR([HAVE_DOT],
	[Variable used by doxygen to declare availibility of dot])
AC_CHECK_TOOL([HAVE_DOT], [dot], [YES], [NO])
AC_ARG_VAR([XMLTO], [Path to xmlto needed to build tutorial documentation])
AC_PATH_TOOL([XMLTO], [xmlto], [noxmlto])
AS_IF([test "$enable_documentation" = "yes" && test "$DOXYGEN" = "nodoxygen" -o "$XMLTO" = "noxmlto"],
	[AC_MSG_ERROR([could not files tools necessary to build documentation])])
AM_CONDITIONAL([BUILD_REFERENCE],
	[test "$enable_documentation" != "no" -a "$DOXYGEN" != "nodoxygen"])
AM_CONDITIONAL([BUILD_TUTORIAL],
	[test "$enable_documentation" != "no" -a "$XMLTO" != "xmlto"])

AM_MAINTAINER_MODE

# See if we want verbose compiler warnings.
AC_MSG_CHECKING([maintainer mode])
AC_ARG_ENABLE(maintainer-mode)
AC_MSG_RESULT(${enable_maintainer_mode})

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
		AC_TRY_COMPILE([], [], has_option=yes, has_option=no,)
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
			-Wwrite-strings  \
			-Wundef \
			-Wunused

		# "Iffy" g++ options.  Some reasonably current g++-like
		# compilers may not support these.
		add_compiler_opts_if_ok \
			-fnothrow-opt \
			-Wattribute-alias \
			-Wextra-semi \
			-Wlogical-op \
			-Wrestrict \
			-Wstringop-overflow \
			-Wsuggest-override \
			-Wzero-as-null-pointer-constant
	fi


AC_MSG_CHECKING([g++ visibility attribute])
gcc_visibility=yes
SAVE_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -Werror"
AC_TRY_COMPILE(
	[
		struct __attribute__ ((visibility("hidden"))) d
		{
			d() {}
			void f() {}
		};
	],
	[d D; D.f()],
	AC_DEFINE(
		[PQXX_HAVE_GCC_VISIBILITY],
		1,
		[Define if g++ supports visibility attribute, as in g++ 4.0]),
	gcc_visibility=no)
AC_MSG_RESULT($gcc_visibility)
CXXFLAGS="$SAVE_CXXFLAGS"
if test "$gcc_visibility" = "yes"
then
    # Make internal definitions accessible only to the library itself.
    # Only definitions marked PQXX_LIBEXPORT will be accessible.
    add_compiler_opts -fvisibility=hidden
    add_compiler_opts -fvisibility-inlines-hidden
fi

# Check gcc "const" attribute.  (More like a stricter version of "pure").
AX_GCC_CONST_CALL
if test "$ax_cv_gcc_const_call"
then
	AC_DEFINE(
		[PQXX_HAVE_GCC_CONST],
		1,
		[Define if g++ supports const attribute.])
fi

if test "$deprecated" != "yes"
then
AC_MSG_CHECKING([g++ deprecation attribute])
gcc_deprecated=yes
AC_TRY_COMPILE(
	[__attribute__ ((deprecated)) void f();],
	[],
	AC_DEFINE(
		[PQXX_HAVE_GCC_DEPRECATED],
		1,
		[Define if g++ supports deprecated attribute, as in g++ 4.0]),
	gcc_deprecated=no)
AC_MSG_RESULT($gcc_deprecated)
fi

AC_MSG_CHECKING([g++ pure attribute])
gcc_pure=yes
AC_TRY_COMPILE(
	[void __attribute__ ((pure)) f();],
	[],
	AC_DEFINE(
		[PQXX_HAVE_GCC_PURE],
		1,
		[Define if g++ supports pure attribute]),
	gcc_pure=yes)
AC_MSG_RESULT($gcc_pure)

fi # End of gcc-specific part.


AC_MSG_CHECKING([C++14 deprecation attribute])
deprecated=yes
AC_TRY_COMPILE(
	[[[deprecated]] void f();],
	[],
	AC_DEFINE(
		[PQXX_HAVE_DEPRECATED],
		1,
		[Define if compiler supports [[deprecated]] attribute]),
	deprecated=no)
AC_MSG_RESULT($deprecated)


AC_MSG_CHECKING([for C++17 std::optional support])
have_optional=yes
AC_TRY_COMPILE(
	[#include <optional>],
	[return std::optional<int>(0).value()],
	AC_DEFINE(
		[PQXX_HAVE_OPTIONAL],
		1,
		[Define if the compiler supports std::optional.]),
	have_optional=no)
AC_MSG_RESULT($have_optional)

if test "$have_optional" != "yes"
then
AC_MSG_CHECKING([for experimental C++17 std::optional support])
have_exp_optional=yes
AC_TRY_COMPILE(
	[#include <experimental/optional>],
	[return std::experimental::optional<int>(0).value()],
	AC_DEFINE(
		[PQXX_HAVE_EXP_OPTIONAL],
		1,
		[Define if the compiler supports std::experimental::optional.]),
	have_exp_optional=no)
AC_MSG_RESULT($have_exp_optional)
fi


AX_HAVE_POLL(
        [AX_CONFIG_FEATURE_ENABLE(poll)],
        [AX_CONFIG_FEATURE_DISABLE(poll)])
AX_CONFIG_FEATURE([poll], [System supports poll().], [HAVE_POLL], [System supports poll().])

if test "$ax_cv_have_poll" != "yes"
then
# No poll(); try for select() instead.
select_h=no
AC_CHECK_HEADERS([sys/select.h], [select_h=yes])

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


AC_CHECK_HEADERS([sys/time.h])


# Find PostgreSQL includes and libraries
AC_PATH_PROGS(PG_CONFIG, pg_config)
if test -z "$PG_CONFIG" || test ! -r "$PG_CONFIG"
then
	AC_MSG_ERROR([
PostgreSQL configuration script pg_config not found.  Make sure this is in your
command path before configuring.  Without it, the configure script has no way to
find the right location for the libpq library and its headers.
])
fi

AC_ARG_WITH(
    [postgres-include],
    [AS_HELP_STRING(
        [--with-postgres-include=DIR],
        [Use PostgreSQL includes from DIR.  Defaults to querying pg_config.])],
    AS_IF(
        [test "x$with_postgres_include" = "xyes"],
        [with_postgres_include=""]))
AS_IF(
    [test -z "$with_postgres_include"],
    [with_postgres_include=$($PG_CONFIG --includedir)])
AC_MSG_NOTICE([using PostgreSQL headers at $with_postgres_include])
AC_SUBST(with_postgres_include)
POSTGRES_INCLUDE="-I${with_postgres_include}"
AC_SUBST(POSTGRES_INCLUDE)

AC_ARG_WITH(
    [postgres-lib],
    [AS_HELP_STRING(
        [--with-postgres-lib=DIR],
        [Use PostgreSQL libraries from DIR.  Defaults to querying pg_config.])],
    AS_IF(
        [test "x$with_postgres_lib" = "xyes"],
        [with_postgres_lib=""]))
AS_IF(
    [test -z "$with_postgres_lib"],
    [with_postgres_lib=$($PG_CONFIG --libdir)])
AC_MSG_NOTICE([using PostgreSQL libraries at $with_postgres_lib])
AC_SUBST(with_postgres_lib)
#POSTGRES_LIB="-R${with_postgres_lib}"
AC_SUBST(POSTGRES_LIB)


AC_CHECK_HEADER(
	[${with_postgres_include}/libpq-fe.h],
	[],
	[AC_MSG_ERROR([
Can't find libpq-fe.h in ${with_postgres_include}.  Are you sure the libpq
headers are installed correctly?  They should be in the directory returned by
"pg_config --includedir".

If you do have libpq (the C-language client library for PostgreSQL) installed,
make sure you have the related development materials--mainly its header files--
as well as the library binary.  Some system distributions keep the two in
seperate packages with names like "alibrary" and "alibrary-dev", respectively.
In that case, make sure you have the latter installed as well.
])])


AC_MSG_CHECKING([for ability to compile source files using libpq])
AC_TRY_COMPILE(
	[#include<${with_postgres_include}/libpq-fe.h>],[PQexec(nullptr,"")],
	[],
	[AC_MSG_ERROR([
Could not compile a call to a basic libpq function.  There must be something
seriously wrong with the headers that "pg_config --includedir" pointed to; the
contents of config.log may give you a clue about the nature of the failure.
Source including the libpq header libpq-fe.h can be compiled, but a call to the
most basic libpq function PQexec() failed to compile successfully.  This is the
litmus test for a working libpq.
])])
AC_MSG_RESULT(yes)


LDFLAGS="$LDFLAGS -L${with_postgres_lib}"

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
	-L${with_postgres_lib})

# Remove redundant occurrances of -lpq
LIBS=[$(echo "$LIBS" | sed -e 's/-lpq * -lpq\>/-lpq/g')]


AC_MSG_CHECKING([that type of libpq's Oid is as expected])
AC_TRY_COMPILE(
	[
		#include<${with_postgres_include}/libpq-fe.h>
		#include"${srcdir}/include/pqxx/internal/libpq-forward.hxx"
		extern void f(pqxx::oid&);
	],
	[Oid o;f(o)],
	[],
	[AC_MSG_ERROR([
The Oid typedef in libpq has changed.  Please notify the libpqxx authors of the
change!
])])
AC_MSG_RESULT(yes)


AC_PROG_MAKE_SET

AC_CONFIG_FILES([
	Makefile config/Makefile doc/Makefile doc/Doxyfile src/Makefile
	test/Makefile test/unit/Makefile tools/Makefile win32/Makefile
	include/Makefile include/pqxx/Makefile libpqxx.pc])


AC_CONFIG_COMMANDS([configitems], ["${srcdir}/tools/splitconfig" "${srcdir}"])

AC_OUTPUT
