# Generate configure script for libpqxx.  This needs the autoconf archive
# package installed.  (The configure script itself does not require it though.)

AC_PREREQ(2.69)
AC_INIT(
	libpqxx,
	[m4_esyscmd_s([./tools/extract_version])],
	[Jeroen T. Vermeulen])
AC_LANG(C++)
# Require C++17 at minimum.
AX_CXX_COMPILE_STDCXX_17([noext])
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


# Read test programme from config-test.
AC_DEFUN([read_test], [AC_LANG_SOURCE(
	esyscmd(tools/m4esc.py --input=config-tests/$1))])


# Checks for programs.
AC_PROG_CXX
AC_PROG_INSTALL
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PATH_PROG([MKDIR], [mkdir])

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

# See if we want "audit," with loads of runtime checking.
AC_MSG_CHECKING([audit])
AC_ARG_ENABLE(audit)
AC_MSG_RESULT(${enable_audit})


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
			-Wattribute-alias=2 \
			-Wextra-semi \
			-Wlogical-op \
			-Wrestrict \
			-Wstringop-overflow \
			-Wsuggest-override \
			-Wzero-as-null-pointer-constant \
			-Warray-bounds=2 \
			-Wduplicated-branches \
			-Wduplicated-cond \
			-Wtrampolines
	fi

	# In "audit," enable all runtime checks we can.
	if test "$enable_audit" = "yes"
	then
		add_compiler_opts_if_ok \
			-fsanitize=address \
			-fsanitize=leak \
			-fsanitize=undefined \
			-fsanitize-address-use-after-scope \
			-fstack-protector-all
	fi


AC_MSG_CHECKING([g++ visibility attribute])
gcc_visibility=yes
SAVE_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS -Werror"
AC_COMPILE_IFELSE(
	[read_test(gcc_visibility.cxx)],
	AC_DEFINE(
		[PQXX_HAVE_GCC_VISIBILITY],
		1,
		[Define if g++ supports visibility attribute.]),
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

AC_MSG_CHECKING([g++ pure attribute])
gcc_pure=yes
AC_COMPILE_IFELSE(
	[read_test(gcc_pure.cxx)],
	AC_DEFINE(
		[PQXX_HAVE_GCC_PURE],
		1,
		[Define if g++ supports pure attribute]),
	gcc_pure=no)
AC_MSG_RESULT($gcc_pure)

fi # End of gcc-specific part.


# Check for __cxa_demangle.
AC_MSG_CHECKING([__cxa_demangle])
cxa_demangle=yes
AC_COMPILE_IFELSE(
	[read_test(cxa_demangle.cxx)],
	AC_DEFINE(
		[PQXX_HAVE_CXA_DEMANGLE],
		1,
		[Define if compiler supports __cxa_demangle]),
	cxa_demangle=no)
AC_MSG_RESULT($cxa_demangle)


# Check for strnlen_s.
AC_MSG_CHECKING([strnlen_s])
strnlen_s=yes
AC_COMPILE_IFELSE(
	[read_test(strnlen_s.cxx)],
	AC_DEFINE(
		[PQXX_HAVE_STRNLEN_S],
		1,
		[Define if compiler provides strnlen_s]),
	strnlen_s=no)
AC_MSG_RESULT($strnlen_s)


# Check for strnlen.
AC_MSG_CHECKING([strnlen])
strnlen=yes
AC_COMPILE_IFELSE(
	[read_test(strnlen.cxx)],
	AC_DEFINE(
		[PQXX_HAVE_STRNLEN],
		1,
		[Define if compiler provides strnlen]),
	strnlen=no)
AC_MSG_RESULT($strnlen)


# It's mid-2019, and gcc's charconv supports integers but not yet floats.
# So for now, we test for int and float conversion... separately.
#
# It's worse for older clang versions, which compile the integer conversions
# but then fail at link time because of a missing symbol "__muloti4" with the
# "long long" version.  I couldn't resolve that symbol by adding -lm either.
# So don't just compile these tests; link them as well.
AC_MSG_CHECKING([for C++17 charconv integer conversion])
have_charconv_int=yes
AC_LINK_IFELSE(
	[read_test(charconv_int.cxx)],
	AC_DEFINE(
		[PQXX_HAVE_CHARCONV_INT],
		1,
		[Define if <charconv> supports integer conversion.]),
	have_charconv_int=no)
AC_MSG_RESULT($have_charconv_int)

AC_MSG_CHECKING([for C++17 charconv floating-point conversion])
have_charconv_float=yes
AC_LINK_IFELSE(
	[read_test(charconv_float.cxx)],
	AC_DEFINE(
		[PQXX_HAVE_CHARCONV_FLOAT],
		1,
		[Define if <charconv> supports floating-point conversion.]),
	have_charconv_float=no)
AC_MSG_RESULT($have_charconv_float)

# Doing my own test for poll() for now.  The test built into autoconf-archive
# triggers stupid warnings.
# TODO: Try newer version of autoconf-archive.
#AX_HAVE_POLL(
#        [AX_CONFIG_FEATURE_ENABLE(poll)],
#        [AX_CONFIG_FEATURE_DISABLE(poll)])
#AX_CONFIG_FEATURE(
#	[poll],
#	[System supports poll().],
#	[PQXX_HAVE_POLL],
#	[System supports poll().])
AC_MSG_CHECKING([for poll()])
ax_cv_have_poll=yes
AC_LINK_IFELSE(
	[read_test(poll.cxx)],
	AC_DEFINE(
		[PQXX_HAVE_POLL],
		1,
		[Define if poll() is available.]),
	ax_cv_have_poll=no)
AC_MSG_RESULT($ax_cv_have_poll)

if test "$ax_cv_have_poll" != "yes"
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
AC_PATH_PROGS(PG_CONFIG, pg_config)
AC_PATH_PROG([PKG_CONFIG], [pkg-config])

AC_ARG_WITH(
    [postgres-include],
    [AS_HELP_STRING(
        [--with-postgres-include=DIR],
        [Use PostgreSQL includes from DIR.  Defaults to querying pg_config or pkg-config, whichever is available.])],
    AS_IF(
        [test "x$with_postgres_include" = "xyes"],
        [with_postgres_include=""]))

if test -n "$PKG_CONFIG" && test -x "$PKG_CONFIG"; then
AS_IF(
    [test -z "$with_postgres_include"],
    [with_postgres_include=$($PKG_CONFIG libpq --cflags | sed 's/^-I//' | sed 's/ *$//')])
AC_MSG_NOTICE([using PostgreSQL headers at $with_postgres_include])
AC_SUBST(with_postgres_include)
POSTGRES_INCLUDE="-I${with_postgres_include}"
AC_SUBST(POSTGRES_INCLUDE)
elif test -n "$PG_CONFIG" && test -x "$PG_CONFIG"; then
AS_IF(
    [test -z "$with_postgres_include"],
    [with_postgres_include=$($PG_CONFIG --includedir)])
AC_MSG_NOTICE([using PostgreSQL headers at $with_postgres_include])
AC_SUBST(with_postgres_include)
POSTGRES_INCLUDE="-I${with_postgres_include}"
AC_SUBST(POSTGRES_INCLUDE)
else
       AC_MSG_ERROR([
Neither PostgreSQL configuration script pg_config nor pkg-config found.
Make sure at least one of these is in your command path before configuring.
Without either of them, the configure script has no way to find the right
location for the libpq library and its headers.
])
fi

AC_ARG_WITH(
    [postgres-lib],
    [AS_HELP_STRING(
        [--with-postgres-lib=DIR],
        [Use PostgreSQL libraries from DIR.  Defaults to querying pg_config.])],
    AS_IF(
        [test "x$with_postgres_lib" = "xyes"],
        [with_postgres_lib=""]))

if test -n "$PKG_CONFIG" && test -x "$PKG_CONFIG"; then
    AS_IF(
       [test -z "$with_postgres_lib"],
       [with_postgres_lib=$($PKG_CONFIG libpq --libs-only-L | sed 's/^-L//')])
    AC_MSG_NOTICE([using PostgreSQL libraries at $with_postgres_lib])
    # configure insists on inserting a library search path, so give him a standard one ...
    AC_SUBST(with_postgres_lib, "")
    #POSTGRES_LIB="-R${with_postgres_lib}"
    AC_SUBST(POSTGRES_LIB)
elif test -n "$PG_CONFIG" && test -x "$PG_CONFIG"; then
    AS_IF(
       [test -z "$with_postgres_lib"],
       [with_postgres_lib=$($PG_CONFIG --libdir)])
    AC_MSG_NOTICE([using PostgreSQL libraries at $with_postgres_lib])
    AC_SUBST(with_postgres_lib)
    #POSTGRES_LIB="-R${with_postgres_lib}"
    AC_SUBST(POSTGRES_LIB)
fi


AC_CHECK_HEADER(
	[${with_postgres_include}/libpq-fe.h],
	[],
	[AC_MSG_ERROR([
Can't find libpq-fe.h in ${with_postgres_include}.  Are you sure the libpq
headers are installed correctly?  They should be in the directory returned by
"pg_config --includedir" or "pkg-config libpq --cflags".

If you do have libpq (the C-language client library for PostgreSQL) installed,
make sure you have the related development materials--mainly its header files--
as well as the library binary.  Some system distributions keep the two in
seperate packages with names like "alibrary" and "alibrary-dev", respectively.
In that case, make sure you have the latter installed as well.
])])


AC_MSG_CHECKING([for ability to compile source files using libpq])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[[#include<${with_postgres_include}/libpq-fe.h>]],
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


# PQencryptPasswordConn was added in postgres 10.
AC_MSG_CHECKING([for PQencryptPasswordConn])
have_pqencryptpasswordconn=yes
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[#include<${with_postgres_include}/libpq-fe.h>],
		[
			extern PGconn *conn;
			PQencryptPasswordConn(
				conn, "pwd", "user", "scram-sha-256")
		]
	)],
	AC_DEFINE(
		[PQXX_HAVE_PQENCRYPTPASSWORDCONN],
		1,
		[Define if libpq has PQencryptPasswordConn (since pg 10).]),
	[have_pqencryptpasswordconn=no])
AC_MSG_RESULT($have_pqencryptpasswordconn)


# Remove redundant occurrances of -lpq
LIBS=[$(echo "$LIBS" | sed -e 's/-lpq * -lpq\>/-lpq/g')]


AC_MSG_CHECKING([that type of libpq's Oid is as expected])
AC_COMPILE_IFELSE(
	[AC_LANG_PROGRAM(
		[
			#include<${with_postgres_include}/libpq-fe.h>
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
	Makefile config/Makefile doc/Makefile doc/Doxyfile src/Makefile
	test/Makefile test/unit/Makefile tools/Makefile include/Makefile
	include/pqxx/Makefile libpqxx.pc])


AC_CONFIG_COMMANDS([configitems], ["${srcdir}/tools/splitconfig" "${srcdir}"])

AC_OUTPUT
