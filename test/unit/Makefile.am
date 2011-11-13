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
# Generated from template '/home/jtv/proj/libpqxx/trunk/test/unit/Makefile.am.template'.
################################################################################
#MAINTAINERCLEANFILES=Makefile.in

EXTRA_DIST = Makefile.am.template

INCLUDES = -I$(top_builddir)/include -I$(top_builddir)/test -I$(srcdir)/.. -I$(srcdir)/../../include ${POSTGRES_INCLUDE}

DEFAULT_INCLUDES=

MAINTAINERCLEANFILES=Makefile.in

runner_SOURCES = \
  test_binarystring.cxx \
  test_cancel_query.cxx \
  test_error_verbosity.cxx \
  test_errorhandler.cxx \
  test_escape.cxx \
  test_exceptions.cxx \
  test_float.cxx \
  test_notification.cxx \
  test_parameterized.cxx \
  test_pipeline.cxx \
  test_prepared_statement.cxx \
  test_read_transaction.cxx \
  test_result_slicing.cxx \
  test_simultaneous_transactions.cxx \
  test_sql_cursor.cxx \
  test_stateless_cursor.cxx \
  test_string_conversion.cxx \
  test_test_helpers.cxx \
  test_thread_safety_model.cxx \
  runner.cxx

runner_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

TESTS = runner
check_PROGRAMS = ${TESTS}

# ###MAKTEMPLATE:FOREACH test/unit/test_*.cxx
# ###BASENAME###_SOURCES = ###BASENAME###.cxx
# ###BASENAME###_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

