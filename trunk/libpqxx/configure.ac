# Process this file with autoconf to produce a configure script.
AC_PREREQ(2.53)
AC_INIT(libpqxx, 0.2.4, [Jeroen T. Vermeulen <jtv@xs4all.nl>])
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE(libpqxx, 0.2.4)
AC_CONFIG_SRCDIR([src/connection.cxx])
AM_CONFIG_HEADER(config.h)

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
# (Omit the -Waggregate-return and -Weffc++ options because they currently
# yield too many warnings in gcc's standard headers)
if test "$GCC" = "yes" && test "$enable_maintainer_mode" = "yes"; then
	for option in \
		-ffor-scope \
		-pedantic \
		-Wall \
		-Wsystem-headers \
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
		-Wmissing-declarations  \
		-Wmissing-noreturn \
		-Wmissing-format-attribute  \
		-Wpacked  \
		-Wpadded \
		-Wredundant-decls \
		-Wunreachable-code \
		-Winline \
		-Wdisabled-optimization \
		-Wctor-dtor-privacy \
		-Wnon-virtual-dtor \
		-Wreorder \
		-Wno-deprecated \
		-Wno-non-template-friend \
		-Wold-style-cast \
		-Woverloaded-virtual \
		-Wno-pmf-conversions \
		-Wsign-promo \
		-Wsynth \
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
AC_LANG_POP(C++)

# does the user want to use postgres-style SQL?
AC_MSG_CHECKING(using PostgreSQL specific SQL)
AH_TEMPLATE(DIALECT_POSTGRESQL,[Use postgres SQL?])
AC_ARG_ENABLE(postgres-dialect,
AC_HELP_STRING(
	[--enable-postgres-dialect]
	, [Use postgres-specific SQL statements (default is yes)])
	,
		if test "$enableval" = yes; then
			AC_DEFINE(DIALECT_POSTGRESQL)
			enable_postgres_dialect=yes
		else
			enable_postgres_dialect=no
		fi
	,
		enable_postgres_dialect=yes
		AC_DEFINE(DIALECT_POSTGRESQL)
)
AC_MSG_RESULT($enable_postgres_dialect)

# Where are the postgres includes...
postgres_include_default=/usr/include
AC_ARG_WITH(postgres-include,
	AC_HELP_STRING(
		[--with-postgres-include],
		[PostgreSQL headers directory]
	)
	,	test "$withval" = no || with_postgres_include=$withval
	, with_postgres_include=$postgres_include_default
)

# ..and libraries?
postgres_lib_default=/usr/lib
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
POSTGRES_LIB="-L${with_postgres_lib} -lpq"
AC_SUBST(POSTGRES_LIB)
POSTGRES_INCLUDE="-I${with_postgres_include} -I${with_postgres_include}/server"
AC_SUBST(POSTGRES_INCLUDE)

# Checks for libraries.
AC_CHECK_LIB([pq], [PQexec], [], [AC_MSG_ERROR([Failed to find a suitable libpq.

PostgreSQL's libpq needs to be installed.
Perhaps you need to specify --with-postgres-lib?
])], -L${with_postgres_lib})

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([errno.h])
AC_CHECK_HEADER([${with_postgres_include}/libpq-fe.h],,AC_MSG_ERROR(
[Can't find libpq-fe.h in ${with_postgres_include}.
Please specify the location of the PostgreSQL include files with
--with-postgres-include=... or --with-postgres=...]
))

AC_LANG_PUSH(C++)
AC_CHECK_HEADERS([limits iterator])

# Checks for typedefs, structures, and compiler characteristics.
AC_MSG_CHECKING(whether <iterator> defines an iterator template)
AC_TRY_COMPILE([#include <iterator>], 
	[struct I:std::iterator<std::random_access_iterator_tag,int>{};],
	[it='yes'],
	[AC_DEFINE(BROKEN_ITERATOR, 
			1, 
			[Define if <iterator> lacks an iterator template definition])
      	 it='no'])
AC_MSG_RESULT($it)
AC_LANG_POP(C++)

AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_STRUCT_TM
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([memset])

AC_CONFIG_FILES([Makefile config/Makefile doc/Makefile src/Makefile test/Makefile include/Makefile])
AC_OUTPUT

