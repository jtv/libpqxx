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
  company \
  # (End of list)

noinst_PROGRAMS = $(EXAMPLES)

# Override automatically generated list of default include directories.  It
# contains only unnecessary entries, and include/pqxx which is wrong.
DEFAULT_INCLUDES=

MAINTAINERCLEANFILES=Makefile.in

company_SOURCES = company.cxx
company_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

AM_CPPFLAGS = -I$(top_builddir)/include -I$(top_srcdir)/include

TESTS = $(EXAMPLES)
