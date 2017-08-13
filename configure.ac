# All text in libpqxx is pure ASCII, so we can afford the "C" locale which is
# likely to be faster than any other.
LC_ALL=C

AC_PREREQ(2.59)
AC_INIT(
	libpqxx,
	[m4_esyscmd_s([./tools/extract_version])],
	[Jeroen T. Vermeulen <jtv@xs4all.nl>])
AC_LANG(C++)
AC_CONFIG_SRCDIR([src/connection.cxx])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config/m4])
AM_INIT_AUTOMAKE

PQXX_ABI=m4_esyscmd_s([./tools/extract_version --abi])
AC_SUBST(PQXXVERSION)
AC_SUBST(PQXX_ABI)

AC_CONFIG_HEADER([include/pqxx/config.h])

# default prefix for installs
AC_PREFIX_DEFAULT(/usr/local)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
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
	[enable_documentation=yes])
if test "$enable_documentation" = "yes"
then
AC_PATH_PROG([DOXYGEN], [doxygen])
AM_CONDITIONAL([BUILD_REFERENCE], [/bin/true])
AC_CHECK_PROG(HAVE_DOT, dot, YES, NO)
AC_PATH_PROG([XMLTO], [xmlto])
AM_CONDITIONAL([BUILD_TUTORIAL], [/bin/true])
else
AM_CONDITIONAL(BUILD_REFERENCE, [/bin/false])
AM_CONDITIONAL(BUILD_TUTORIAL, [/bin/false])
fi


AM_MAINTAINER_MODE

# see if we want verbose compiler warnings
AC_MSG_CHECKING([maintainer mode])
AC_ARG_ENABLE(maintainer-mode)
AC_MSG_RESULT(${enable_maintainer_mode})

AC_ARG_ENABLE(shared)
AS_IF(
    [test "${shared}" = "yes" ],
    [CPPFLAGS="$CPPFLAGS -DPQXX_SHARED"])


# Add options to compiler command line, if compiler accepts it
add_compiler_opts_if_ok() {
	for option in $* ; do
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
if test "$GCC" = "yes" ; then
	# In maintainer mode, enable all the warning options we can.
	if test "$enable_maintainer_mode" = "yes"; then
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
			-Wformat-security \
			-Winit-self \
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
			-fnothrow-opt
	fi

	# The PostgreSQL headers now require "long long".
	add_compiler_opts_if_ok -Wno-long-long

AC_MSG_CHECKING([g++ visibility attribute])
gcc_visibility=yes
SAVE_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -Werror"
AC_TRY_COMPILE([
struct __attribute__ ((visibility("hidden"))) d { d() {} void f() {} };
	],
	[d D;D.f()],
	AC_DEFINE([PQXX_HAVE_GCC_VISIBILITY],1,
		[Define if g++ supports visibility attribute, as in g++ 4.0]),
	gcc_visibility=no)
AC_MSG_RESULT($gcc_visibility)
CXXFLAGS="$SAVE_CXXFLAGS"
if test "$gcc_visibility" = "yes" ; then
    # Make internal definitions accessible only to the library itself.
    # Only definitions marked PQXX_LIBEXPORT will be accessible.
    add_compiler_opts -fvisibility=hidden
    add_compiler_opts -fvisibility-inlines-hidden 
fi

AC_MSG_CHECKING([g++ const attribute])
gcc_const_attr=yes
AC_TRY_COMPILE([void __attribute__ ((const)) f();],
	[],
	AC_DEFINE([PQXX_HAVE_GCC_CONST], 1,
		[Define if g++ supports const attribute]),
	gcc_const_attr=yes)
AC_MSG_RESULT($gcc_const_attr)

AC_MSG_CHECKING([C++14 deprecation attribute])
deprecated=yes
AC_TRY_COMPILE([[[deprecated]] void f();],
	[],
	AC_DEFINE([PQXX_HAVE_DEPRECATED],1,
		[Define if compiler supports [[deprecated]] attribute]),
	deprecated=no)
AC_MSG_RESULT($deprecated)

if test "$deprecated" != "yes" ; then
AC_MSG_CHECKING([g++ deprecation attribute])
gcc_deprecated=yes
AC_TRY_COMPILE([__attribute__ ((deprecated)) void f();],
	[],
	AC_DEFINE([PQXX_HAVE_GCC_DEPRECATED],1,
		[Define if g++ supports deprecated attribute, as in g++ 4.0]),
	gcc_deprecated=no)
AC_MSG_RESULT($gcc_deprecated)
fi

AC_MSG_CHECKING([g++ pure attribute])
gcc_pure=yes
AC_TRY_COMPILE([void __attribute__ ((pure)) f();],
	[],
	AC_DEFINE([PQXX_HAVE_GCC_PURE], 1,
		[Define if g++ supports pure attribute]),
	gcc_pure=yes)
AC_MSG_RESULT($gcc_pure)

fi


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

if test "$have_optional" != "yes" ; then
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


# Find PostgreSQL includes and libraries
AC_PATH_PROGS(PG_CONFIG, pg_config)
if test -z "$PG_CONFIG" || test ! -r "$PG_CONFIG"; then
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
        [test "x$with_postgres_include" == "xyes"],
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
        [test "x$with_postgres_lib" == "xyes"],
        [with_postgres_lib=""]))
AS_IF(
    [test -z "$with_postgres_lib"],
    [with_postgres_lib=$($PG_CONFIG --libdir)])
AC_MSG_NOTICE([using PostgreSQL libraries at $with_postgres_lib])
AC_SUBST(with_postgres_lib)
#POSTGRES_LIB="-R${with_postgres_lib}"
AC_SUBST(POSTGRES_LIB)


AC_HEADER_STDC


# We should be able to link socket-based programs.
socklibok=no
AC_SEARCH_LIBS(select, socket nsl ws2_32 wsock32 winsock, [socklibok=yes])

# Microsoft proprietary libraries do not work with code that is generated with
# autoconf's SEARCH_LIBS macro, so we need to check manually and just use the
# first socket library available.
# We only do this if select() is not available by other means, to avoid picking
# up an unnecessary Windows compatibility library on a non-Windows system.
for l in ws2_32 wsock32 winsock ; do
	if test "${socklibok}" != "yes" ; then
		AC_CHECK_LIB($l,main,LIBS="$LIBS -l$l";[socklibok=yes])
	fi
done

if test "${socklibok}" != "yes" ; then
	AC_MSG_ERROR([
Could not figure out how to link a simple sockets-based program.  Please read
the config.log file for more clues as to why this failed.
])
fi


AC_CHECK_HEADER([${with_postgres_include}/libpq-fe.h],[],[AC_MSG_ERROR([
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
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>],[PQexec(0,"")],
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


# Perform this check in C; autoconf's AC_HAVE_LIBRARY breaks in C++ because it
# tries to call main()
AC_LANG_PUSH(C)

LDFLAGS="$LDFLAGS -L${with_postgres_lib}"

AC_HAVE_LIBRARY(pq, [], [AC_MSG_ERROR([
Could not link to libpq.  Make sure you have the PostgreSQL client library
installed, and that the library binary can be found in the location returned by
"pg_config --libdir".
])], -L${with_postgres_lib})

AC_CHECK_LIB([pq], [PQexec],[], [AC_MSG_ERROR([
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
])], -L${with_postgres_lib})

# Remove redundant occurrances of -lpq
LIBS=[$(echo "$LIBS" | sed -e 's/-lpq * -lpq\>/-lpq/g')]

AC_LANG_POP(C)


AC_MSG_CHECKING([for correct C++ linkage of basic libpq functions])
AC_LINK_IFELSE([
    AC_LANG_SOURCE([[
#include <${with_postgres_include}/libpq-fe.h>
int main(){return !PQexec(0,0);}
]])],
[], 
[AC_MSG_ERROR([
Linking a call to libpq failed in C++, even though it succeeded in C.  If your
C and C++ compilers are very different beasts, this may mean that we do not have
the right options for linking with it after all.  Alternatively, this may be
caused by a bug in autoconf or automake.

If you are configuring libpqxx using the --enable-maintainer-mode option, any
compiler warnings for autoconf test programs will be treated as errors.  The
problem may just go away if you run the configure script again but without the
maintainer-mode option.  Please report such cases (including the config.log
produced by the failed attempt) so future versions can attempt to work around
the problem.

Should this error occur even while not in maintainer mode, read the config.log
file for more detailed information.  Look for the last error message, which may
be several pages up from the end of the file.
])])
AC_MSG_RESULT(yes)


AC_MSG_CHECKING([that type of libpq's Oid is as expected])
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>
	#include"${srcdir}/include/pqxx/internal/libpq-forward.hxx"
	extern void f(pqxx::oid&);],
	[Oid o;f(o)],
	[],
	[AC_MSG_ERROR([
The Oid typedef in libpq has changed.  Please notify the libpqxx authors of the
change!
])])
AC_MSG_RESULT(yes)

AC_MSG_CHECKING([for strerror_r])
strerror_r=yes
AC_TRY_COMPILE(
	[#include <cstring>
	using namespace std;
	bool f(char x[]) { return strerror_r(0,x,10) != 0; }],
	[],
	[AC_DEFINE(PQXX_HAVE_STRERROR_R,1,[Define if strerror_r exists])],
	[strerror_r=no])
AC_MSG_RESULT($strerror_r)
if test "$strerror_r" != "yes" ; then
	AC_MSG_WARN([
No definition of strerror_r, the thread-safe version of strerror(), was found in
your <cstring> header.

This may be because your implementation strerror() is threadsafe, in which case
there is nothing to worry about.
])
else
AC_MSG_CHECKING([for GNU-style strerror_r])
gnu_strerror_r=yes
AC_TRY_COMPILE(
	[#include <cstring>
	using namespace std;
	const char *it_is_a_string(char *x) { return strerror_r(0,x,10); }],
	[],
	[AC_DEFINE(
		PQXX_HAVE_STRERROR_R_GNU,1,[Define for GNU-style strerror_r])],
	[gnu_strerror_r=no])
AC_MSG_RESULT($gnu_strerror_r)
fi # strerror_r


AC_MSG_CHECKING([for strlcpy])
strlcpy=yes
AC_TRY_COMPILE(
	[#include <cstring>
	using namespace std;],
	[char buf[2];return strlcpy(buf,"",1)],
	[AC_DEFINE(PQXX_HAVE_STRLCPY,1,[Define if strlcpy exists])],
	strlcpy=no)
AC_MSG_RESULT($strlcpy)

AC_MSG_CHECKING([for working <sys/select.h>])
select_h=yes
AC_TRY_COMPILE(
	[#include <cstring>
#include <sys/select.h>
using namespace std;]
	, [select(0,0,0,0,0)]
	, AC_DEFINE(PQXX_HAVE_SYS_SELECT_H,1,
[Define if the sys/select.h header defines a working select() and friends, as per POSIX 1003.1-2001])
	,select_h=no
)
AC_MSG_RESULT($select_h)


AC_MSG_CHECKING([for poll()])
poll=yes
AC_TRY_COMPILE(
	[#include <poll.h>
using namespace std;],
	[pollfd p = { 0,0,0 }; poll(&p,1,0)],
	AC_DEFINE(PQXX_HAVE_POLL,1,
[Define if the system has the poll() function (mainly GNU/Linux)]),
	poll=no
)
AC_MSG_RESULT($poll)


AC_MSG_CHECKING([if select() accepts NULL fdsets])
select_accepts_null=yes
AC_RUN_IFELSE([
    AC_LANG_SOURCE([[
#ifdef PQXX_HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <ctime>
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef _WIN32
#include <winsock2.h>
#endif
using namespace std;
extern "C" int foo(){fd_set f; FD_SET(1,&f); return select(2,0,&f,&f,0)<0;}
int main() { return foo(); }
]])],
	AC_DEFINE(PQXX_SELECT_ACCEPTS_NULL,1,
[Define if select() accepts NULL fd_set arguments]),
	select_accepts_null=no,
	select_accepts_null=no
)
AC_MSG_RESULT($select_accepts_null)


AC_PROG_MAKE_SET

AC_CONFIG_FILES([Makefile config/Makefile doc/Makefile
doc/Doxyfile src/Makefile test/Makefile test/unit/Makefile tools/Makefile
win32/Makefile include/Makefile include/pqxx/Makefile libpqxx.pc pqxx-config])


AC_CONFIG_COMMANDS([configitems], ["${srcdir}/tools/splitconfig" "${srcdir}"])

AC_OUTPUT

