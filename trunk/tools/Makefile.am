EXTRA_DIST = \
	extract_version \
	lint \
	maketemporary \
	preprelease \
	release \
	rmlo.cxx \
	splitconfig \
	template2mak.py \
	pqxxthreadsafety.cxx

INCLUDES=-I$(top_builddir)/include -I$(top_srcdir)/include ${POSTGRES_INCLUDE}

noinst_PROGRAMS = rmlo pqxxthreadsafety

rmlo_SOURCES = rmlo.cxx
rmlo_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

pqxxthreadsafety_SOURCES = pqxxthreadsafety.cxx
pqxxthreadsafety_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
