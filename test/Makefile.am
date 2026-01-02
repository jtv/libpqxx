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
# Generated from template './test/Makefile.am.template'.
# ##############################################################################
# Makefile.am is generated automatically for automake whenever test programs are
# added to libpqxx.

EXTRA_DIST = Makefile.am.template

# Use the serial test runner, so tests don't get run in parallel.  Otherwise,
# output from test failures will be hidden away in log files where we can't
# see them when running in a build slave.
AUTOMAKE_OPTIONS=serial-tests


AM_CPPFLAGS=-I$(top_builddir)/include -I$(top_srcdir)/include

# Override automatically generated list of default include directories.  It
# contains only unnecessary entries, and include/pqxx which is wrong.
DEFAULT_INCLUDES=

noinst_HEADERS = helpers.hxx

CLEANFILES=pqxxlo.txt
MAINTAINERCLEANFILES=Makefile.in

runner_SOURCES = \
  test00.cxx \
  test01.cxx \
  test02.cxx \
  test07.cxx \
  test10.cxx \
  test11.cxx \
  test13.cxx \
  test18.cxx \
  test20.cxx \
  test21.cxx \
  test26.cxx \
  test29.cxx \
  test30.cxx \
  test32.cxx \
  test37.cxx \
  test39.cxx \
  test46.cxx \
  test56.cxx \
  test60.cxx \
  test61.cxx \
  test62.cxx \
  test70.cxx \
  test71.cxx \
  test72.cxx \
  test74.cxx \
  test75.cxx \
  test76.cxx \
  test77.cxx \
  test82.cxx \
  test84.cxx \
  test88.cxx \
  test89.cxx \
  test90.cxx \
  test_array.cxx \
  test_blob.cxx \
  test_cancel_query.cxx \
  test_column.cxx \
  test_composite.cxx \
  test_connection.cxx \
  test_connection_string.cxx \
  test_cursor.cxx \
  test_encodings.cxx \
  test_error_verbosity.cxx \
  test_errorhandler.cxx \
  test_escape.cxx \
  test_exceptions.cxx \
  test_field.cxx \
  test_float.cxx \
  test_helpers.cxx \
  test_largeobject.cxx \
  test_nonblocking_connect.cxx \
  test_notice_handler.cxx \
  test_notification.cxx \
  test_params.cxx \
  test_pipeline.cxx \
  test_prepared_statement.cxx \
  test_range.cxx \
  test_read_transaction.cxx \
  test_result_iteration.cxx \
  test_row.cxx \
  test_separated_list.cxx \
  test_simultaneous_transactions.cxx \
  test_sql_cursor.cxx \
  test_stateless_cursor.cxx \
  test_strconv.cxx \
  test_stream_from.cxx \
  test_stream_query.cxx \
  test_stream_to.cxx \
  test_string_conversion.cxx \
  test_subtransaction.cxx \
  test_thread_safety_model.cxx \
  test_time.cxx \
  test_transaction.cxx \
  test_transaction_base.cxx \
  test_transaction_focus.cxx \
  test_transactor.cxx \
  test_type_name.cxx \
  test_util.cxx \
  test_zview.cxx \
  runner.cxx

runner_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

TESTS = runner
check_PROGRAMS = ${TESTS}
