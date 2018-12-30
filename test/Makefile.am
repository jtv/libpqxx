################################################################################
# AUTOMATICALLY GENERATED FILE -- DO NOT EDIT.
#
# This file is generated automatically by libpqxx's template2mak.py script, and
# will be rewritten from time to time.
#
# If you modify this file, chances are your modifications will be lost.
#
# The template2mak.py script should be available in the tools directory of the
# libpqxx source archive.
#
# Generated from template './test/Makefile.am.template'.
################################################################################
# Makefile.am is generated automatically for automake whenever test programs are
# added to libpqxx.
SUBDIRS = unit

EXTRA_DIST = Makefile.am.template

# Use the serial test runner, so tests don't get run in parallel.  Otherwise,
# output from test failures will be hidden away in log files where we can't
# see them when running in a build slave.
AUTOMAKE_OPTIONS=serial-tests


AM_CPPFLAGS=-I$(top_builddir)/include -I$(top_srcdir)/include

# Override automatically generated list of default includes.  It contains only
# unnecessary entries, and incorrectly mentions include/pqxx directly.
DEFAULT_INCLUDES=

noinst_HEADERS = test_helpers.hxx test_main.hxx

CLEANFILES=pqxxlo.txt
MAINTAINERCLEANFILES=Makefile.in

#TESTS_ENVIRONMENT=PGDATABASE=libpqxx
# PGDATABASE, PGHOST, PGPORT, PGUSER

runner_SOURCES = \
  test00.cxx \
  test01.cxx \
  test02.cxx \
  test04.cxx \
  test07.cxx \
  test10.cxx \
  test11.cxx \
  test12.cxx \
  test13.cxx \
  test14.cxx \
  test15.cxx \
  test16.cxx \
  test17.cxx \
  test18.cxx \
  test20.cxx \
  test21.cxx \
  test23.cxx \
  test26.cxx \
  test29.cxx \
  test30.cxx \
  test31.cxx \
  test32.cxx \
  test33.cxx \
  test34.cxx \
  test35.cxx \
  test36.cxx \
  test37.cxx \
  test39.cxx \
  test46.cxx \
  test48.cxx \
  test49.cxx \
  test50.cxx \
  test51.cxx \
  test52.cxx \
  test53.cxx \
  test54.cxx \
  test55.cxx \
  test56.cxx \
  test57.cxx \
  test58.cxx \
  test59.cxx \
  test60.cxx \
  test61.cxx \
  test62.cxx \
  test63.cxx \
  test64.cxx \
  test65.cxx \
  test66.cxx \
  test67.cxx \
  test69.cxx \
  test70.cxx \
  test71.cxx \
  test72.cxx \
  test73.cxx \
  test74.cxx \
  test75.cxx \
  test76.cxx \
  test77.cxx \
  test78.cxx \
  test79.cxx \
  test82.cxx \
  test84.cxx \
  test86.cxx \
  test87.cxx \
  test88.cxx \
  test89.cxx \
  test90.cxx \
  test92.cxx \
  test93.cxx \
  runner.cxx

runner_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

TESTS = runner
check_PROGRAMS = ${TESTS}

# ###MAKTEMPLATE:FOREACH test/test*.cxx
# ###BASENAME###_SOURCES = ###BASENAME###.cxx
# ###BASENAME###_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
