EXTRA_DIST = \
	maketestam.pl \
	maketestvcmak.pl \
	pqxxbench.cxx pqxxbench.in \
	preprelease \
	rmlo.cxx

INCLUDES=-I$(top_srcdir)/include ${POSTGRES_INCLUDE}

noinst_PROGRAMS = pqxxbench rmlo

pqxxbench_SOURCES = pqxxbench.cxx
pqxxbench_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

rmlo_SOURCES = rmlo.cxx
rmlo_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

