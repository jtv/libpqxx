EXTRA_DIST = \
	deprecations \
	extract_version \
	format \
	generate_check_config.py \
	generate_cxx_checks.py \
	lint \
	m4esc.py \
	pqxxthreadsafety.cxx \
	rmlo.cxx \
	splitconfig.py \
	template2mak.py \
	todo \
	update-copyright

AM_CPPFLAGS=-I$(top_builddir)/include -I$(top_srcdir)/include ${POSTGRES_INCLUDE}
# Override automatically generated list of default includes.  It contains only
# unnecessary entries, and incorrectly mentions include/pqxx directly.
DEFAULT_INCLUDES=

noinst_PROGRAMS = rmlo pqxxthreadsafety

rmlo_SOURCES = rmlo.cxx
rmlo_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

pqxxthreadsafety_SOURCES = pqxxthreadsafety.cxx
pqxxthreadsafety_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
