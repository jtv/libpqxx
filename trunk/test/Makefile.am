################################################################################
# AUTOMATICALLY GENERATED FILE--DO NOT EDIT
#
# This file is generated automatically by libpqxx's template2mak.py script.
#
# If you modify this file, chances are your modifications will be lost because
# the file will be re-written from time to time.
#
# The template2mak.py script should be available in the tools directory of the
# libpqxx source archive.
#
# Generated from template '/home/jtv/proj/libpqxx/trunk/test/Makefile.am.template'.
################################################################################
# Makefile.am is generated automatically for automake whenever test programs are
# added to libpqxx.
SUBDIRS = unit

EXTRA_DIST = Makefile.am.template

INCLUDES=-I$(top_builddir)/include -I$(top_srcdir)/include

# Override automatically generated list of default includes.  It contains only
# unnecessary entries, and incorrectly mentions include/pqxx directly.
DEFAULT_INCLUDES=

noinst_HEADERS = test_helpers.hxx test_main.hxx

CLEANFILES=pqxxlo.txt
MAINTAINERCLEANFILES=Makefile.in

#TESTS_ENVIRONMENT=PGDATABASE=libpqxx
# PGDATABASE, PGHOST, PGPORT, PGUSER

runner_SOURCES = \
  test000.cxx \
  test001.cxx \
  test002.cxx \
  test004.cxx \
  test007.cxx \
  test010.cxx \
  test011.cxx \
  test012.cxx \
  test013.cxx \
  test014.cxx \
  test015.cxx \
  test016.cxx \
  test017.cxx \
  test018.cxx \
  test020.cxx \
  test021.cxx \
  test023.cxx \
  test026.cxx \
  test029.cxx \
  test030.cxx \
  test031.cxx \
  test032.cxx \
  test033.cxx \
  test034.cxx \
  test035.cxx \
  test036.cxx \
  test037.cxx \
  test039.cxx \
  test046.cxx \
  test048.cxx \
  test049.cxx \
  test050.cxx \
  test051.cxx \
  test052.cxx \
  test053.cxx \
  test054.cxx \
  test055.cxx \
  test056.cxx \
  test057.cxx \
  test058.cxx \
  test059.cxx \
  test060.cxx \
  test061.cxx \
  test062.cxx \
  test063.cxx \
  test064.cxx \
  test065.cxx \
  test066.cxx \
  test067.cxx \
  test069.cxx \
  test070.cxx \
  test071.cxx \
  test072.cxx \
  test073.cxx \
  test074.cxx \
  test075.cxx \
  test076.cxx \
  test077.cxx \
  test078.cxx \
  test079.cxx \
  test082.cxx \
  test083.cxx \
  test084.cxx \
  test086.cxx \
  test087.cxx \
  test088.cxx \
  test089.cxx \
  test090.cxx \
  test092.cxx \
  test093.cxx \
  test094.cxx \
  runner.cxx

runner_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

TESTS = runner
check_PROGRAMS = ${TESTS}

# ###MAKTEMPLATE:FOREACH test/test*.cxx
# ###BASENAME###_SOURCES = ###BASENAME###.cxx
# ###BASENAME###_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
