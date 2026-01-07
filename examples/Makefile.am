# ##############################################################################
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
# Generated from template './examples/Makefile.am.template'.
# ##############################################################################
# Template for generating Makefile.am.
EXTRA_DIST = Makefile.am.template

EXAMPLES = \
  complete_example \
  faster_data \
  from_readme \
  getting_started_1 \
  getting_started_2 \
  quick_example \
  simple_queries \
  # (End of list)

noinst_PROGRAMS = $(EXAMPLES)

# Override automatically generated list of default include directories.  It
# contains only unnecessary entries, and include/pqxx which is wrong.
DEFAULT_INCLUDES=

MAINTAINERCLEANFILES=Makefile.in

complete_example_SOURCES = complete_example.cxx
complete_example_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
faster_data_SOURCES = faster_data.cxx
faster_data_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
from_readme_SOURCES = from_readme.cxx
from_readme_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
getting_started_1_SOURCES = getting_started_1.cxx
getting_started_1_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
getting_started_2_SOURCES = getting_started_2.cxx
getting_started_2_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
quick_example_SOURCES = quick_example.cxx
quick_example_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
simple_queries_SOURCES = simple_queries.cxx
simple_queries_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include

TESTS = $(EXAMPLES)
