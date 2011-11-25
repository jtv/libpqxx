#
# CAUTION: THE configure.ac FILE IS AUTOMATICALLY GENERATED!  DO NOT EDIT IT.
#
# If you want to make changes to configure.ac, edit configure.ac.in instead.
#
# Process configure.ac.in with autoconf to produce a configure script.

# All text in libpqxx is pure ASCII, so we can afford the "C" locale which is
# likely to be faster than any other.  This may also work around bugs in some
# environments, esp. with UTF-8 locales.
LC_ALL=C

AC_PREREQ(2.59)
AC_INIT(libpqxx, 4.0, [Jeroen T. Vermeulen <jtv@xs4all.nl>])
AC_LANG(C++)
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR([config/m4])
AM_INIT_AUTOMAKE(libpqxx, 4.0)

PQXXVERSION=4.0
PQXX_ABI=4.0
PQXX_MAJOR=4
PQXX_MINOR=0
AC_SUBST(PQXXVERSION)
AC_SUBST(PQXX_ABI)
AC_SUBST(PQXX_MAJOR)
AC_SUBST(PQXX_MINOR)

AC_CONFIG_SRCDIR([src/connection.cxx])
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
if test "${shared}" = "yes" ; then
	CPPFLAGS="$CPPFLAGS -DPQXX_SHARED"
fi

# Add options to compiler command line, if compiler accepts it
add_compiler_opts() {
	for option in $* ; do
		ACO_SAVE_CXXFLAGS="$CXXFLAGS"
		CXXFLAGS="$CXXFLAGS $option"
		AC_MSG_CHECKING([whether $CXX accepts $option])
		AC_TRY_COMPILE([], [], has_option=yes, has_option=no,)
		AC_MSG_RESULT($has_option)
		if test "$has_option" = "no" ; then
			CXXFLAGS="$ACO_SAVE_CXXFLAGS"
		fi
	done
}


# Let's try to get the compiler to be helpful
# (Omit options -Weffc++ and -Wabi because they currently yield too many
# warnings in gcc's standard headers; omit -Wunreachable-code because it isn't
# always right)
if test "$GCC" = "yes" ; then
	# In maintainer mode, enable all the warning options we can
	if test "$enable_maintainer_mode" = "yes"; then
		add_compiler_opts \
			-Werror \
			-Wno-div-by-zero \
			-fnothrow-opt \
			-ffor-scope \
			-fstrict-enums \
			-fstrict-aliasing \
			-Wstrict-aliasing \
			-funit-at-a-time \
			-pedantic \
			-fno-nonansi-builtins \
			-Wall \
			-Wmultichar \
			-W \
			-Wextra \
			-Wfloat-equal \
			-Wundef \
			-Wshadow \
			-Wpointer-arith  \
			-Wcast-qual  \
			-Wcast-align  \
			-Wconversion \
			-Wctor-dtor-privacy \
			-Wendif-labels \
			-Wlogical-op \
			-Woverlength-strings \
			-Wredundant-decls \
			-Wsign-compare \
			-Wwrite-strings  \
			-Wnon-virtual-dtor \
			-Wreorder \
			-Wold-style-cast \
			-Woverloaded-virtual \
			-Wsign-promo \
			-Wstrict-null-sentinel \
			-Wformat-security \
			-Winit-self \
			-Wswitch \
			-Wmissing-field-initializers \
			-Wmissing-include-dirs \
			-Wunused
	fi

	if test "$enable_long_long" = "yes" ; then
		add_compiler_opts -Wno-long-long
	fi

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
	if test "$gcc_visibility" = "yes" ; then
		# Setting default symbol visibility to "hidden" vastly reduces
		# library size with g++ 4.0, but unfortunately this will require
		# some more work in the libpqxx setup.  Instead, we make only
		# inline functions hidden.
		add_compiler_opts -fvisibility-inlines-hidden 
		#add_compiler_opts -fvisibility=hidden
	fi
CXXFLAGS="$SAVE_CXXFLAGS"

AC_MSG_CHECKING([g++ const attribute])
gcc_const_attr=yes
AC_TRY_COMPILE([void __attribute__ ((const)) f();],
	[],
	AC_DEFINE([PQXX_HAVE_GCC_CONST], 1,
		[Define if g++ supports const attribute]),
	gcc_const_attr=yes)
AC_MSG_RESULT($gcc_const_attr)

AC_MSG_CHECKING([g++ deprecation attribute])
gcc_deprecated=yes
AC_TRY_COMPILE([void __attribute__ ((deprecated)) f();],
	[],
	AC_DEFINE([PQXX_HAVE_GCC_DEPRECATED],1,
		[Define if g++ supports deprecated attribute, as in g++ 4.0]),
	gcc_deprecated=no)
AC_MSG_RESULT($gcc_deprecated)

AC_MSG_CHECKING([g++ noreturn attribute])
gcc_noreturn=yes
# Needs a throw spec and a definition in order to trip up a bug in clang++ 2.8.
# It'll claim the throw spec is missing on the definition.
AC_TRY_COMPILE([
	void __attribute__ ((noreturn)) f() throw (int);
        void f()throw(int){throw 1;}
	], [],
	AC_DEFINE([PQXX_HAVE_GCC_NORETURN],1,
		[Define if g++ supports noreturn attribute]),
	gcc_noreturn=no)
AC_MSG_RESULT($gcc_noreturn)

AC_MSG_CHECKING([g++ pure attribute])
gcc_pure=yes
AC_TRY_COMPILE([void __attribute__ ((pure)) f();],
	[],
	AC_DEFINE([PQXX_HAVE_GCC_PURE], 1,
		[Define if g++ supports pure attribute]),
	gcc_pure=yes)
AC_MSG_RESULT($gcc_pure)

fi

# Determine name of standard namespace.  PGSTD will be set to this name.  The
# --with-std= option takes precedence if given; otherwise, several alternatives
# are tried.
# As a special case, if the argument to the --with-std option is "yes" (as would
# be the case if the user specified the option but no argument for it), the
# guessing logic is enabled as if the option had not been used.
AH_TEMPLATE(PGSTD,[name of standard library namespace (normally "std")])
AC_MSG_CHECKING([name of standard library namespace (normally "std")])
guessstd=no
AC_ARG_WITH(std,
	AC_HELP_STRING(
		[--with-std],
		[name of standard library namespace (normally "std")]),
	[std=${withval};],
	[guessstd=yes])

if test "${guessstd}" = "yes" -o "${withval}" = "yes"; then
	# No explicit std namespace given.  Check for alternative "standard"
	# namespaces, in order of preference.
	for ns in std _STL stlp_std "" ; do
		AC_COMPILE_IFELSE([#include <string>
			${ns}::string justastring;],
			[std="${ns}"; break])
	done
fi
AC_DEFINE_UNQUOTED(
	PGSTD,
	[$std],
	[name of standard library namespace (normally "std")])
if ! test -z "${std}" ; then
	AC_MSG_RESULT([::${std}])
	usestd="namespace ${std} {} using namespace ${std};"
else
	AC_MSG_RESULT([::])
fi


boost_smart_ptr=yes
AC_CHECK_HEADER(boost/smart_ptr.hpp, AC_DEFINE(PQXX_HAVE_BOOST_SMART_PTR, 1,
	[Define if you have the <boost/smart_ptr.hpp> header]),
	boost_smart_ptr=no)


AC_MSG_CHECKING([whether TR1 headers live in tr1 directory])
tr1headers=yes
AC_TRY_COMPILE(
	[#include <tr1/memory>],
	[],
	AC_DEFINE(
		[PQXX_TR1_HEADERS],
		1,
		[Define if headers for TR1 extension to standard live in tr1/]),
	tr1headers=no)
AC_MSG_RESULT($tr1headers)

if test "${tr1headers}" = "yes" ; then
	tr1dir="tr1/"
else
	tr1dir=""
fi

AH_TEMPLATE(PQXXTR1, [namespace of TR1 extension to the C++ standard])
AC_MSG_CHECKING([namespace of TR1 extension to the C++ standard])
guesstr1=no
AC_ARG_WITH(
	tr1,
	AC_HELP_STRING(
		[--with-tr1],
		[namespace of TR1 extension to the C++ standard]),
	[tr1=${withval};],
	[guesstr1=yes])

if test "${guesstr1}" = "yes" ; then
	# No explicit TR1 namespace given.  Try PGSTD::tr1.  If that doesn't
	# work, assume PGSTD as our best bet.
	tr1="${std}::tr1"
	AC_TRY_COMPILE(
		[#include <${tr1dir}memory>],
		[using namespace ${tr1}],
		[],
		[tr1=${std}])
fi
AC_DEFINE_UNQUOTED(
	PQXXTR1,
	[$tr1],
	[namespace of TR1 standard extensions (e.g. "std" or "std::tr1")])
AC_MSG_RESULT([$tr1])

ioshdr=yes
AC_CHECK_HEADER(ios,AC_DEFINE(PQXX_HAVE_IOS,1,
[Define if you have the <ios> header]),
ioshdr=no)

streambuf=yes
AC_CHECK_HEADER(streambuf,AC_DEFINE(PQXX_HAVE_STREAMBUF,1,
[Define if you have the <streambuf> header; if not, <streambuf.h> is used]),
streambuf=no)

localehdr=yes
AC_CHECK_HEADER(locale,AC_DEFINE(PQXX_HAVE_LOCALE,1,
[Define if you have the <locale> header]),
localehdr=no)

imbue=yes
AC_MSG_CHECKING([locale function and stringstream::imbue])
AC_TRY_COMPILE([#include <sstream>
	#ifdef PQXX_HAVE_LOCALE
	#include <locale>
	#endif
	${usestd}],
	[stringstream S; S.imbue(locale("C"))],
	[AC_DEFINE(PQXX_HAVE_IMBUE, 1, [Define if stringstream::imbue works])],
	[imbue=no])
AC_MSG_RESULT($imbue)

AC_MSG_CHECKING([char_traits template])
char_traits=yes
AC_TRY_COMPILE([#include <string>
	${usestd}], 
	[char_traits<char>::eof()],
	AC_DEFINE(PQXX_HAVE_CHAR_TRAITS, 1,
[Define if std provides the char_traits template and <char> specialization]),
char_traits=no)
AC_MSG_RESULT($char_traits)

AC_MSG_CHECKING(["warning" preprocessor directive])
cpp_warning=yes
AC_TRY_COMPILE([#warning cpp supports warning directive], [], AC_DEFINE(PQXX_HAVE_CPP_WARNING, 1,
[Define if preprocessor supports nonstandard "warning" directive]),
cpp_warning=no)
AC_MSG_RESULT($cpp_warning)

AC_MSG_CHECKING(["message" preprocessor pragma])
cpp_pragma_message=yes
AC_TRY_COMPILE([#pragma message("cpp supports message pragma")], [], AC_DEFINE(PQXX_HAVE_CPP_PRAGMA_MESSAGE, 1,
[Define if preprocessor supports pragma "message"]),
cpp_pragma_message=no)
AC_MSG_RESULT($cpp_pragma_message)

# Find PostgreSQL includes and libraries
AC_PATH_PROGS(PG_CONFIG, pg_config)
if test -z "$PG_CONFIG" || test ! -r "$PG_CONFIG"; then
	AC_MSG_ERROR([
PostgreSQL configuration script pg_config not found.  Make sure this is in your
command path before configuring.  Without it, the configure script has no way to
find the right location for the libpq library and its headers.
])
fi
with_postgres_include=`$PG_CONFIG --includedir`
AC_MSG_NOTICE([using PostgreSQL headers at $with_postgres_include])
with_postgres_lib=`$PG_CONFIG --libdir`
AC_MSG_NOTICE([using PostgreSQL libraries at $with_postgres_lib])

AC_SUBST(with_postgres_lib)
AC_SUBST(with_postgres_include)
#POSTGRES_LIB="-R${with_postgres_lib}"
AC_SUBST(POSTGRES_LIB)
POSTGRES_INCLUDE="-I${with_postgres_include}"
AC_SUBST(POSTGRES_INCLUDE)


AC_HEADER_STDC

AC_MSG_CHECKING([ability to compile programs using the standard C library])
AC_TRY_COMPILE([#include <string.h>
	${usestd}
	extern void f(size_t);],
	[f(strlen(""))],[],
	[AC_MSG_ERROR([
Could not build even a tiny test program using the standard C library.

Something very basic has gone wrong; try reading config.log to find out what.
You'll probably want to go straight to the end of that file and read backwards
until you find an error message.  Then identify the problem, try to fix it, and
run this script again.

The configure script's --help option may provide useful tips on how to influence
what compiler and command line options are used, what additional libraries are
linked in, and so on.
])])
AC_MSG_RESULT(yes)


# After this check, we should be able to link socket-based programs.
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
library file is damaged, or in some incorrect format. or if your libpq is much
more recent than libpqxx version 4.0, perhaps libpq has undergone a
radical ABI change.

The last parts of config.log may give you a clue as to what really went wrong,
but be warned that this is no easy reading.  Look for the last error message
occurring in the file.
])], -L${with_postgres_lib})

# Remove redundant occurrances of -lpq
LIBS="`echo "$LIBS" | sed -e 's/-lpq[[:space:]]*[[:space:]]-lpq\>/-lpq/g'`"

AC_LANG_POP(C)

AC_MSG_CHECKING([for correct C++ linkage of basic libpq functions])
if test "$enable_maintainer_mode" = "yes"; then
AC_LINK_IFELSE([
#include <${with_postgres_include}/libpq-fe.h>
int main(){return !PQexec(0,0);}],
[], 
[AC_MSG_ERROR([
Linking a call to libpq failed in C++, even though it succeeded in C.  If your
C and C++ compilers are very different beasts, this may mean that we do not have
the right options for linking with it after all.  Alternatively, this may be
caused by a bug in autoconf or automake.

Since you are configuring libpqxx using the --enable-maintainer-mode option, any
compiler warnings for autoconf test programs will be treated as errors.  The
problem may just go away if you run the configure script again but with the
maintainer mode option omitted.  Please report such cases (including the
config.log produced by the failed attempt) so future versions can attempt to
work around the problem.

Should this error occur even while not in maintainer mode, read the config.log
file for more detailed information.  Look for the last error message, which may
be several pages up from the end of the file.
])])
else
AC_LINK_IFELSE([
#include <${with_postgres_include}/libpq-fe.h>
int main(){return !PQexec(0,0);}],
[], 
[AC_MSG_ERROR([
Linking a call to libpq failed in C++, even though it succeeded in C.  If your
C and C++ compilers are very different beasts, this may mean that we do not have
the right options for linking with it after all.

Read the config.log file for more detailed information.  Look for the last error
message, which may be several pages up from the end of the file.
])])
fi
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

AC_MSG_CHECKING([lo_tell()])
lotell=yes
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>
	void f(PGconn *c) { lo_tell(c,2); }],
	[],
	AC_DEFINE(PQXX_HAVE_LO_TELL, 1,
		[Define if libpq has lo_tell()]),
	[lotell=no]
)
AC_MSG_RESULT($lotell)


AC_MSG_CHECKING([for strerror_r])
strerror_r=yes
AC_TRY_COMPILE(
	[#include <cstring>
	${usestd}
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
	${usestd}
	const char *it_is_a_string(char *x) { return strerror_r(0,x,10); }],
	[],
	[AC_DEFINE(
		PQXX_HAVE_STRERROR_R_GNU,1,[Define for GNU-style strerror_r])],
	[gnu_strerror_r=no])
AC_MSG_RESULT($gnu_strerror_r)
fi # strerror_r


AC_MSG_CHECKING([for strnlen])
strnlen=yes
AC_TRY_COMPILE(
	[#include <cstring>
	${usestd}],
	[return strnlen("",1)],
	[AC_DEFINE(PQXX_HAVE_STRNLEN,1,[Define if strnlen exists])],
	strnlen=no)
AC_MSG_RESULT($strnlen)

AC_MSG_CHECKING([for strlcpy])
strlcpy=yes
AC_TRY_COMPILE(
	[#include <cstring>
	${usestd}],
	[char buf[2];return strlcpy(buf,"",1)],
	[AC_DEFINE(PQXX_HAVE_STRLCPY,1,[Define if strlcpy exists])],
	strlcpy=no)
AC_MSG_RESULT($strlcpy)

AC_MSG_CHECKING([for long long])
long_long=yes
AC_TRY_COMPILE(,[long long l=0LL; return int(l)],
	AC_DEFINE(PQXX_HAVE_LONG_LONG,1,
	[Define if the compiler supports "long long" types]),
	long_long=no)
AC_MSG_RESULT($long_long)

AC_MSG_CHECKING([for long double])
long_double=yes
AC_TRY_COMPILE(,[long double x=0;return int(x)],
	AC_DEFINE(PQXX_HAVE_LONG_DOUBLE,1,
	[Define if the compiler supports the standard "long double" type]),
	long_double=no)
AC_MSG_RESULT($long_double)

AC_MSG_CHECKING([for working <sys/select.h>])
select_h=yes
AC_TRY_COMPILE(
	[#include <cstring>
#include <sys/select.h>
${usestd}]
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
${usestd}],
	[pollfd p = { 0,0,0 }; poll(&p,1,0)],
	AC_DEFINE(PQXX_HAVE_POLL,1,
[Define if the system has the poll() function (mainly GNU/Linux)]),
	poll=no
)
AC_MSG_RESULT($poll)


# Long-standing annoyance in glibc: the definition for FD_SET includes an
# unnecessary C-style cast that the compiler may warn for.  If the compiler is
# configured to treat warnings as errors, that may be a problem for us.
AC_MSG_CHECKING([for working fd_set])
if test "$enable_maintainer_mode" = "yes"; then
AC_TRY_COMPILE(
[#include <cstring>
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
${usestd}
extern "C" void f() { fd_set s; FD_ZERO(&s); FD_SET(1, &s); }],
	[],
	[],
	[AC_MSG_ERROR([
Could not compile code using fd_set.  This may be a known problem with GNU libc
and the extremely strict compiler options used in libpqxx's maintainer mode; try
configuring again, this time omitting the --enable-maintainer-mode option.
])])
else
AC_TRY_COMPILE(
[#include <cstring>
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
${usestd}],
	[fd_set s;FD_ZERO(&s);FD_SET(1,&s)],
	[],
	[AC_MSG_ERROR([
Could not compile code using fd_set.  There is something seriously wrong, or
maybe your warning options are too strict.  If you are using GNU libc (as will
be the case on a GNU/Linux system) and are telling the compiler to treat
warnings as errors, the cause may be an old-style cast in the definition of
FD_SET that triggers a compiler warning.

See config.log for more details; look for the last error message in the file.
])])
fi
AC_MSG_RESULT(yes)


AC_MSG_CHECKING([if select() accepts NULL fdsets])
select_accepts_null=yes
AC_RUN_IFELSE(
[#ifdef PQXX_HAVE_SYS_SELECT_H
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
${usestd}
extern "C" int foo(){fd_set f; FD_SET(1,&f); return select(2,0,&f,&f,0)<0;}
int main() { return foo(); }],
	AC_DEFINE(PQXX_SELECT_ACCEPTS_NULL,1,
[Define if select() accepts NULL fd_set arguments]),
	select_accepts_null=no,
	select_accepts_null=no
)
AC_MSG_RESULT($select_accepts_null)


AC_MSG_CHECKING([sleep()])
sleep=yes
AC_TRY_COMPILE([#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
	${usestd}],[sleep(0x01)],
	AC_DEFINE(PQXX_HAVE_SLEEP, 1, [Define if POSIX sleep() exists]),
	[sleep=no]
)
AC_MSG_RESULT($sleep)

AC_MSG_CHECKING([PQisthreadsafe()])
pqisthreadsafe=yes
AC_TRY_COMPILE(
	[#include<${with_postgres_include}/libpq-fe.h>],
	[PQisthreadsafe()],
	AC_DEFINE(
		PQXX_HAVE_PQISTHREADSAFE,
		1,
		[Define if libpq has PQisthreadsafe()]),
	[pqisthreadsafe=no])
AC_MSG_RESULT($pqisthreadsafe)

AC_MSG_CHECKING([PQescapeLiteral()])
pqescapeliteral=yes
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>],
	[PQescapeLiteral(0,"x",1)],
	AC_DEFINE(PQXX_HAVE_PQESCAPELITERAL, 1,
		[Define if libpq has PQescapeLiteral()]),
	[pqescapeliteral=no]
)
AC_MSG_RESULT($pqescapeliteral)


AC_MSG_CHECKING([support for hex binary escaping format])
	pqunescapebytea_9=yes
	AC_RUN_IFELSE([
#include<stdio.h>
#include<${with_postgres_include}/libpq-fe.h>
int main()
{
  typedef unsigned char *ucharptr;
  size_t sz = 0;
  /* Pre-9.0 octal escaping will unescape this as "x20". */
  const unsigned char *foo;
  foo = PQunescapeBytea(ucharptr("\\\\x20"), &sz);
  return *foo != 0x20;
}
],
		AC_DEFINE(PQXX_HAVE_PQUNESCAPEBYTEA_9, 1,
[Define if PQunescapeBytea() supports hex escape format]),
		pqunescapebytea_9=no,
		pqunescapebytea_9=no)
AC_MSG_RESULT($pqunescapebytea_9)


AC_MSG_CHECKING([PQescapeIdentifier()])
pqescapeidentifier=yes
AC_TRY_COMPILE(
	[#include<${with_postgres_include}/libpq-fe.h>],
	[PQescapeIdentifier(NULL, "foo", 3)],
	AC_DEFINE(
		PQXX_HAVE_PQESCAPEIDENTIFIER,
		1,
		[Define if libpq has PQescapeIdentifier()]),
	[pqescapeidentifier=no]
)
AC_MSG_RESULT(pqescapeidentifier)


AC_MSG_CHECKING([PQencryptPassword()])
pqencryptpassword=yes
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>],
	[PQencryptPassword("x","y")],
	AC_DEFINE(PQXX_HAVE_PQENCRYPTPASSWORD,1,[Define if libpq has PQencryptPassword()]),
	[pqencryptpassword=no]
)
AC_MSG_RESULT($pqencryptpassword)


AC_MSG_CHECKING([PQmblen()])
pqmblen=yes
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>],
	[return PQmblen(NULL,1)],
	AC_DEFINE(PQXX_HAVE_PQMBLEN,1,[Define if libpq has PQmblen()]),
	[pqmblen=no]
)
AC_MSG_RESULT($pqmblen)


AC_MSG_CHECKING([PQdescribePortal()])
pqdescribeportal=yes
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>],
	[return PQdescribePortal(NULL, "foo")!=0;],
	AC_DEFINE(PQXX_HAVE_PQDESCRIBEPORTAL,1,[Define if libpq has PQdescribePortal()]),
	[pqdescribeportal=no]
)
AC_MSG_RESULT($pqdescribeportal)

AC_MSG_CHECKING([PQclientEncoding()])
pqclientencoding=yes
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>],
	[PQclientEncoding(0)],
	AC_DEFINE(PQXX_HAVE_PQCLIENTENCODING, 1,
		[Define if libpq has PQclientEncoding()]),
	[pqclientencoding=no]
)
AC_MSG_RESULT($pqclientencoding)

AC_MSG_CHECKING([PQcancel()])
pqcancel=yes
AC_TRY_COMPILE([#include<${with_postgres_include}/libpq-fe.h>],
	[PQcancel(NULL, NULL, 0)],
	AC_DEFINE(PQXX_HAVE_PQCANCEL, 1,
		[Define if libpq has PQcancel() and friends]),
	[pqcancel=no]
)
AC_MSG_RESULT($pqcancel)

AC_CHECK_HEADER(limits,
	[AC_DEFINE(PQXX_HAVE_LIMITS, 1, [Define if <limits> exists])])

AC_MSG_CHECKING([for C99 isnan()])
c_isnan=yes
AC_TRY_COMPILE([#include <cmath>
	extern double f();
	${usestd}],
	[if(isnan(f())f()],
	AC_DEFINE(PQXX_HAVE_C_ISNAN,1,[Define if math.h defines C99 isnan()]),
	[c_isnan=no]
)
AC_MSG_RESULT($c_isnan)

AC_MSG_CHECKING([for std::numeric_limits<>::quiet_NaN()])
quiet_nan=yes
AC_TRY_COMPILE([#include <limits>
	extern void f(double);
	${usestd}],
	[f(numeric_limits<double>::quiet_NaN())],
	[AC_DEFINE(PQXX_HAVE_QUIET_NAN,1,
		[Define if std::numeric_limits has quiet_NaN()])],
	[quiet_nan=no])
AC_MSG_RESULT($quiet_nan)

AC_MSG_CHECKING([for C NAN macro])
c_nan=yes
AC_TRY_COMPILE([#include <cmath>
	extern void f(double);
	${usestd}],
	[f(NAN)],
	[AC_DEFINE(PQXX_HAVE_C_NAN,1,
		[Define if C math header defines NAN])],
	[c_nan=no])
AC_MSG_RESULT($c_nan)

AC_MSG_CHECKING([for C99 nan() family])
nan=yes
AC_TRY_COMPILE([#include <cmath>
	${usestd}
	extern void f(double);],
	[f(nan(""))],
	[AC_DEFINE(PQXX_HAVE_NAN,1,
		[Define if C99 fnan()/nan()/lnan() are available])],
	[nan=no])
AC_MSG_RESULT($nan)

AC_MSG_CHECKING([for C99 isinf()])
c_isnan=yes
AC_TRY_COMPILE([#include <cmath>
	extern double f();
	${usestd}],
	[if(isinf(f())f()],
	AC_DEFINE(PQXX_HAVE_C_ISINF,1,[Define if math.h defines C99 isinf()]),
	[c_isinf=no]
)
AC_MSG_RESULT($c_isinf)

AC_MSG_CHECKING([whether std::string has a clear() function])
string_clear=yes
AC_TRY_COMPILE([#include <string>
	${usestd}],
	[string i;i.clear()],
	[AC_DEFINE(PQXX_HAVE_STRING_CLEAR,
			1,
			[Define if string class has the clear() function])],
	[string_clear=no])
AC_MSG_RESULT($string_clear)

AC_MSG_CHECKING([for standard distance()])
distance=yes
AC_TRY_COMPILE([#include <iterator>
	${usestd}],
	[int i[1]; return distance(i, i+1)],
	[AC_DEFINE(PQXX_HAVE_DISTANCE,
		1,
		[Define if distance() works according to the standard])],
	[distance='no'])
AC_MSG_RESULT($distance)

AC_MSG_CHECKING([if count_if() works as expected])
count_if='yes'
AC_TRY_COMPILE([#include <algorithm>
#include <functional>
${usestd}
struct P{P(){}bool operator()(const int&)const{return false;}}p;],
	[int v;return int(count_if(&v,&v,p))],
	[AC_DEFINE(PQXX_HAVE_COUNT_IF,
			1,
			[Define if count_if() works according to the standard])],
	[count_if='no'])
AC_MSG_RESULT($count_if)

AC_MSG_CHECKING([whether <iterator> defines a usable iterator template])
AC_TRY_COMPILE([#include <iterator>
	${usestd}],
	[struct I:iterator<random_access_iterator_tag,int>{}],
	[it='yes'],
	[AC_DEFINE(PQXX_BROKEN_ITERATOR,
			1,
			[Define if <iterator> lacks an iterator template definition])
	 it='no'])
AC_MSG_RESULT($it)

AC_MSG_CHECKING([for reverse_iterator template])
reverseit='yes'
AC_TRY_COMPILE([#include <iterator>
	${usestd} struct I:iterator<random_access_iterator_tag,int>{};],
	[reverse_iterator<I> rit],
	[AC_DEFINE(PQXX_HAVE_REVERSE_ITERATOR,
		1,
		[Define if reverse_iterator template works as expected])],
	[reverseit='no'])
AC_MSG_RESULT($reverseit)

AC_MSG_CHECKING([for auto_ptr])
auto_ptr='yes'
AC_TRY_COMPILE([
	#include <memory>
	${usestd}
        int *get_ptr();],
	[auto_ptr<int> i(get_ptr())],
	[AC_DEFINE(PQXX_HAVE_AUTO_PTR,
		1,
		[Define if compiler has auto_ptr])],
	[auto_ptr='no'])
AC_MSG_RESULT($auto_ptr)

AC_MSG_CHECKING([for tr1::shared_ptr])
sharedptr='yes'
AC_TRY_COMPILE([#include <${tr1dir}memory>],
	[${tr1}::shared_ptr<int> i(new int(1)); return *i;],
	[AC_DEFINE(PQXX_HAVE_SHARED_PTR,
		1,
		[Define if compiler has shared_ptr])],
	[sharedptr='no'])
AC_MSG_RESULT($sharedptr)

AC_MSG_CHECKING([for unique_ptr])
unique_ptr='yes'
AC_TRY_COMPILE([
	#include <memory>
	${usestd}
	int *get_ptr();],
	[unique_ptr<int> i(get_ptr())],
	[AC_DEFINE(PQXX_HAVE_UNIQUE_PTR,
		1,
		[Define if compiler has unique_ptr])],
	[unique_ptr='no'])
AC_MSG_RESULT($unique_ptr)

AC_MSG_CHECKING([for move()])
move='yes'
AC_TRY_COMPILE([
	#include <utility>
	${usestd}
	int foo(int &i) { return move(i); }],
	[int i = 0; return foo(i);],
	[AC_DEFINE(PQXX_HAVE_MOVE,
		1,
		[Define if compiler has move().])],
	[move='no'])
AC_MSG_RESULT($move)

AC_MSG_CHECKING([whether overloaded using-declarations work])
AC_TRY_COMPILE([
	struct A {A(){} void f(int){}};
	struct B:A {B(){} using A::f; void f(int,bool){}};],
	[B b;b.f(1,true);],
	[usingdecl='yes'],
	[AC_DEFINE(PQXX_BROKEN_USING_DECL,
		1,
		[Define if function overloading using "using" breaks])
	 usingdecl='no'])
AC_MSG_RESULT($usingdecl)


AC_PROG_MAKE_SET

AC_CONFIG_FILES([Makefile config/Makefile debian/Makefile doc/Makefile
doc/Doxyfile src/Makefile test/Makefile test/unit/Makefile tools/Makefile
win32/Makefile include/Makefile include/pqxx/Makefile libpqxx.pc pqxx-config
libpqxx.spec])


AC_CONFIG_COMMANDS([configitems], ["${srcdir}/tools/splitconfig" "${srcdir}"])

AC_OUTPUT

