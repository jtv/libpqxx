# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(libpqxx, 1.9.1, [Jeroen T. Vermeulen <jtv@xs4all.nl>])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(libpqxx, 1.9.1)

# Library version numbering - see "info libtool versioning 'libtool
# versioning'" for details.
abi_current=0
abi_revision=1
abi_age=0
sonumber=${abi_current}:${abi_revision}:${abi_age}
AC_SUBST(sonumber)

AC_CONFIG_SRCDIR([src/connection.cxx])
AM_CONFIG_HEADER(include/pqxx/config.h)

# default prefix for installs
AC_PREFIX_DEFAULT(/usr/local/pqxx)

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_RANLIB


# check that std namespace is actually called std
# not sure how to check for other namespaces...
AH_TEMPLATE(PGSTD, [name of standard namespace])
AC_LANG_PUSH(C++)
AC_MSG_CHECKING([name of standard namespace])
AC_TRY_COMPILE(
	[#include <string>]
	, [std::string a;]
	, AC_DEFINE(PGSTD,[std],[Does the standard namespace exist?])
	, AC_MSG_ERROR([Can't find std namespace])
)
AC_MSG_RESULT($std)

AM_MAINTAINER_MODE

# see if we want verbose compiler warnings
AC_MSG_CHECKING([maintainer mode])
AC_ARG_ENABLE(maintainer-mode,
	AC_HELP_STRING(
		[--enable-maintainer-mode],
		[Use this for verbose and pedantic compiler warnings. Not for normal builds. (default is no)]
	)
)
AC_MSG_RESULT(${enable_maintainer_mode})

# Let's try to get the compiler to be helpful
# (Omit options -Weffc++ and -Wabi because they currently yield too many 
# warnings in gcc's standard headers; omit -Wunreachable-code because it isn't 
# always right)
if test "$GCC" = "yes" && test "$enable_maintainer_mode" = "yes"; then
	for option in \
		-ffor-scope \
		-pedantic \
		-Wall \
		-Wmultichar \
		-W \
		-Wfloat-equal \
		-Wundef \
		-Wshadow \
		-Wpointer-arith  \
		-Wcast-qual  \
		-Wcast-align  \
		-Wwrite-strings  \
		-Wconversion \
		-Wsign-compare \
		-Wredundant-decls \
		-Wctor-dtor-privacy \
		-Wnon-virtual-dtor \
		-Wreorder \
		-Wold-style-cast \
		-Woverloaded-virtual \
		-Wsign-promo \
		-Wanal \
		-ffascist \
	; do
		SAVE_CXXFLAGS="$CXXFLAGS"
		CXXFLAGS="$CXXFLAGS $option"
		AC_MSG_CHECKING([whether $CXX understands $option])
		AC_TRY_COMPILE([], [],
			has_option=yes,
			has_option=no,)
		if test $has_option = no; then
			CXXFLAGS="$SAVE_CXXFLAGS"
		fi
		AC_MSG_RESULT($has_option)
		unset has_option
		unset SAVE_CXXFLAGS
	done
	unset option
fi

streambuf=yes
AC_CHECK_HEADER(streambuf,AC_DEFINE(HAVE_STREAMBUF,1,
[Define if you have the <streambuf> header; if not, <streambuf.h> is used]),
streambuf=no)
# AC_MSG_RESULT($streambuf)

AC_MSG_CHECKING([char_traits template])
char_traits=yes
AC_TRY_COMPILE([#include <string>], [PGSTD::char_traits<char>::eof()], AC_DEFINE(HAVE_CHAR_TRAITS, 1,
[Define if std defines the char_traits template and <char> specialization]),
char_traits=no)
AC_MSG_RESULT($char_traits)


AC_LANG_POP(C++)

# Where are the postgres includes...
postgres_include_default=
for usual_suspect in \
	/usr/local/include/postgresql \
	/usr/local/include/pgsql \
	/usr/local/pgsql/include \
	/usr/local/include \
	/usr/include/postgresql \
	/usr/include/pgsql \
	/usr/include \
	/opt/postgresql/include \
	/opt/pgsql/include \
; do
AC_CHECK_FILE([$usual_suspect/libpq-fe.h],
	[postgres_include_default=$usual_suspect],
)
	if test -n "$postgres_include_default" ; then
		break
	fi
done

AC_ARG_WITH(postgres-include,
	AC_HELP_STRING(
		[--with-postgres-include],
		[PostgreSQL headers directory]
	)
	,	test "$withval" = no || with_postgres_include=$withval
	, with_postgres_include=$postgres_include_default
)

# ..and libraries?
postgres_lib_default=
for usual_suspect in \
	/usr/local/lib/postgresql \
	/usr/local/lib/pgsql \
	/usr/local/pgsql/lib \
	/usr/local/lib \
	/usr/lib/postgresql \
	/usr/lib/pgsql \
	/usr/lib \
	/opt/postgresql/lib \
	/opt/pgsql/lib \
; do
	for library_extension in so a sa ; do
AC_CHECK_FILE([$usual_suspect/libpq.$library_extension],
	[postgres_lib_default=$usual_suspect],
)
	done
	if test -n "$postgres_lib_default" ; then
		break
	fi
done

AC_ARG_WITH(postgres-lib,
	AC_HELP_STRING(
		[--with-postgres-lib],
		[PostgreSQL library directory containing libpq]
	)
	,	test "$withval" = no || with_postgres_lib=$withval
	, with_postgres_lib=$postgres_lib_default
)

# provide one way to specify both include and lib
AC_ARG_WITH(postgres,
	AC_HELP_STRING(
		[--with-postgres],
		[PostgreSQL common directory. Use this to specify libs & includes rooted at one place]
	)
	,	test "$withval" = no || with_postgres=$withval
		with_postgres_lib=${with_postgres}/lib
		with_postgres_include=${with_postgres}/include
	, with_postgres=no
)

AC_SUBST(with_postgres_lib)
AC_SUBST(with_postgres_include)
POSTGRES_LIB="-L${with_postgres_lib} -R${with_postgres_lib} -lpq"
AC_SUBST(POSTGRES_LIB)
POSTGRES_INCLUDE="-I${with_postgres_include}"
AC_SUBST(POSTGRES_INCLUDE)

# Checks for libraries.
AC_CHECK_LIB([pq], [PQexec], [], [AC_MSG_ERROR([Failed to find a suitable libpq.

PostgreSQL's libpq needs to be installed.
Perhaps you need to specify --with-postgres-lib?
])], -L${with_postgres_lib})

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADER([${with_postgres_include}/libpq-fe.h],,AC_MSG_ERROR(
[Can't find libpq-fe.h in ${with_postgres_include}.
Please specify the location of the PostgreSQL include files with
--with-postgres-include=... or --with-postgres=...]
))

AC_CHECK_HEADER([${with_postgres_include}/libpq/libpq-fs.h],,AC_MSG_ERROR(
[Can't find libpq-fs.h in ${with_postgres_include}/libpq.
Please locate it and report your findings so this script can be corrected.]
))

AC_MSG_CHECKING([for working <sys/select.h>])
select_h=yes
AC_TRY_COMPILE(
	[#include <sys/select.h>]
	, [select(0,0,0,0,0)]
	, AC_DEFINE(HAVE_SYS_SELECT_H,1,
[Define if the sys/select.h header defines a working select() and friends, as per POSIX 1003.1-2001])
	,select_h=no
)
AC_MSG_RESULT($select_h)

AC_MSG_CHECKING([InvalidOid])
AH_TEMPLATE(InvalidOid,[Do we have InvalidOid definition?])
invalidoid=no
AC_TRY_COMPILE(
	[#include <${with_postgres_include}/libpq-fe.h>]
	, [return InvalidOid]
	, [invalidoid=yes]
	, [AC_DEFINE(InvalidOid,0)]
)
AC_MSG_RESULT($invalidoid)

AC_MSG_CHECKING([PQescapeString()])
AH_TEMPLATE(PQescapeString,[Do we have a PQescapeString function?])
pqescapestring=yes
AC_CHECK_DECL(PQescapeString, 
	[AC_DEFINE(HAVE_PQESCAPESTRING, 1, [Define if PostgreSQL has PQescapeString()])],
	[pqescapestring=no],
	[#include <${with_postgres_include}/libpq-fe.h]>)

AC_MSG_CHECKING([PQfreemem()])
AH_TEMPLATE(PQfreemem,[Do we have a PQfreemem function?])
pqfreemem=yes
AC_CHECK_DECL(PQfreemem,
	[AC_DEFINE(HAVE_PQFREEMEM, 1, [Define if PostgreSQL has PQfreemem()])],
	[pqfreemem=no],
	[#include <${with_postgres_include}/libpq-fe.h]>)

AC_LANG_PUSH(C++)
AC_CHECK_HEADERS([limits iterator])

# Checks for typedefs, structures, and compiler characteristics.
AC_MSG_CHECKING([whether <iterator> defines a usable iterator template])
AC_TRY_COMPILE([#include <iterator>], 
	[struct I:PGSTD::iterator<PGSTD::random_access_iterator_tag,int>{}],
	[it='yes'],
	[AC_DEFINE(BROKEN_ITERATOR, 
			1, 
			[Define if <iterator> lacks an iterator template definition])
      	 it='no'])
AC_MSG_RESULT($it)

# See if abs() works on a long int
AC_MSG_CHECKING([for abs(long int)])
AC_TRY_COMPILE([#include <cstdlib>]
               [#include <cmath>],
	[using namespace PGSTD;long a=-2000000000; long b=abs(a);],
	[AC_DEFINE(HAVE_ABS_LONG,
		1,
		[Define if abs() works on long ints])
	 abslong='yes'],
	[abslong='no'])
AC_MSG_RESULT($abslong)

AC_LANG_POP(C++)

AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

AC_PROG_MAKE_SET

AC_CONFIG_FILES([Makefile config/Makefile doc/Makefile src/Makefile
test/Makefile include/Makefile include/pqxx/Makefile libpqxx.pc])
AC_OUTPUT

