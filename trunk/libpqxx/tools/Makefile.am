EXTRA_DIST = \
	maketestam.pl \
	maketestvcmak.pl \
	pqxxbench.cxx pqxxbench.in \
	preprelease \
	rmlo.cxx

INCLUDES=-I$(top_srcdir)/include ${POSTGRES_INCLUDE}
pqxxbenchdir = .
pqxxbench_SOURCES = pqxxbench.cxx
pqxxbench_LDADD = ../src/libpqxx.la ${POSTGRES_LIB}
pqxxbench_PROGRAMS = pqxxbench


