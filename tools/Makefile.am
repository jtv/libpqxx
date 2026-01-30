EXTRA_DIST = \
	deprecations.sh \
	extract_version.sh \
	format.sh \
	generate_check_config.py \
	generate_cxx_checks.py \
	lint.sh \
	m4esc.py \
	pqxxthreadsafety.cxx \
	rmlo.cxx \
	template2mak.py \
	todo.sh \
	update-copyright.sh

AM_CPPFLAGS=-I$(top_builddir)/include -I$(top_srcdir)/include ${POSTGRES_INCLUDE}
# Override automatically generated list of default includes.  It contains only
# unnecessary entries, and incorrectly mentions include/pqxx directly.
DEFAULT_INCLUDES=

noinst_PROGRAMS = rmlo pqxxthreadsafety

rmlo_SOURCES = rmlo.cxx
rmlo_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

pqxxthreadsafety_SOURCES = pqxxthreadsafety.cxx
pqxxthreadsafety_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
