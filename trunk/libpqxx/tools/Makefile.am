EXTRA_DIST = \
	makemingwmak.pl \
	maketestam.pl \
	maketestvcmak.pl \
	makevcmake.pl \
	preprelease \
	rmlo.cxx

INCLUDES=-I$(top_builddir)/include -I$(top_srcdir)/include ${POSTGRES_INCLUDE}

noinst_PROGRAMS = rmlo

rmlo_SOURCES = rmlo.cxx
rmlo_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

