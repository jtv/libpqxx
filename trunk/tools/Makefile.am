EXTRA_DIST = \
	extract_version \
	maketemporary \
	preprelease \
	release \
	rmlo.cxx \
	splitconfig \
	template2mak.py

INCLUDES=-I$(top_builddir)/include -I$(top_srcdir)/include ${POSTGRES_INCLUDE}

noinst_PROGRAMS = rmlo

rmlo_SOURCES = rmlo.cxx
rmlo_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

