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
# Generated from template './Makefile.am.template'.
# ##############################################################################
# Template for generating the root Makefile.am.
#
# We do all the compiling from this central root-level Makefile.am, even
# though the actual source files are distributed across several subdirectories.
# This way we don't waste time waiting to start on the next build target while
# the ongoing target is still being built in its own subdirectory.

# subdir-objects: store the object files in their source file's respective
# subdirectories.
#
# serial-tests: Use the serial test runner, so tests don't get run in parallel.
# Otherwise, output from test failures will be hidden away in log files where
# we can't see them when running in a build slave.
AUTOMAKE_OPTIONS = subdir-objects serial-tests

# We should probably get all the targets in this Makefile.am built (".") before
# doing anything like installing the headers.  I'm not sure it matters.
SUBDIRS = . include

EXTRA_DIST = autogen.sh configitems Makefile.am.template README.md VERSION \
	requirements.json .clang-format .clang-tidy .cmake-format \
	.inferconfig .lcovrc .lgtm.yml .lift .mdlrc .mdl_style.rb .yamllint \
	doc/Doxyfile \
	src/pqxx-source.hxx \
	test/helpers.hxx test/sample_types.hxx \
	tools/benchmark.cxx \
	tools/check_ascii.py \
	tools/compiler_flags.py \
	tools/extract_version.py \
	tools/generate_check_config.py \
	tools/generate_cxx_checks.py \
	tools/m4esc.py \
	tools/template2mak.py \
	tools/deprecations.sh \
	tools/extract_version.sh \
	tools/format.sh \
	tools/idock.sh \
	tools/install-deps.sh \
	tools/lint.sh \
	tools/run-test-postgres.sh \
	tools/todo.sh \
	tools/update-copyright.sh \
	config/flags/audit-mode.txt \
	config/flags/maintainer-mode.txt \
	config/flags/suggest-mode.txt \
	# (End of list)

noinst_HEADERS = test/helpers.hxx

tools_benchmark_SOURCES = tools/benchmark.cxx
tools_benchmark_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}


DISTCLEANFILES = include/pqxx/config-compiler.h

MAINTAINERCLEANFILES = \
    Makefile.in aclocal.m4 config.h.in config.log configure stamp-h.in \
    config/config.guess config/config.sub config/install-sh config/ltmain.sh \
    config/missing config/mkinstalldirs \
    include/pqxx/Makefile.in include/pqxx/stamp-h.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libpqxx.pc

PQXX_TEST_PROGS = test/runner

# We use README.md, but automake expects plain README.
README: README.md
	ln -s $< $@


AM_CPPFLAGS = \
    -I$(top_srcdir)/include -I$(top_builddir)/include ${POSTGRES_INCLUDE}


# Override automatically generated list of default includes.  It contains only
# unnecessary entries, and incorrectly mentions include/pqxx directly.
DEFAULT_INCLUDES=
lib_LTLIBRARIES = src/libpqxx.la
src_libpqxx_la_SOURCES = \
	src/array.cxx \
	src/blob.cxx \
	src/connection.cxx \
	src/cursor.cxx \
	src/encodings.cxx \
	src/errorhandler.cxx \
	src/except.cxx \
	src/field.cxx \
	src/largeobject.cxx \
	src/notification.cxx \
	src/params.cxx \
	src/pipeline.cxx \
	src/result.cxx \
	src/robusttransaction.cxx \
	src/sql_cursor.cxx \
	src/strconv.cxx \
	src/stream_from.cxx \
	src/stream_to.cxx \
	src/subtransaction.cxx \
	src/time.cxx \
	src/transaction.cxx \
	src/transaction_base.cxx \
	src/row.cxx \
	src/types.cxx \
	src/util.cxx \
	src/wait.cxx

libpqxx_version = -release $(PQXX_ABI)

src_libpqxx_la_LDFLAGS = $(libpqxx_version) \
	-rpath $(libdir) \
	${POSTGRES_LIB}

test_runner_SOURCES = \
  test/test00.cxx \
  test/test01.cxx \
  test/test02.cxx \
  test/test07.cxx \
  test/test10.cxx \
  test/test11.cxx \
  test/test13.cxx \
  test/test18.cxx \
  test/test20.cxx \
  test/test21.cxx \
  test/test26.cxx \
  test/test29.cxx \
  test/test30.cxx \
  test/test32.cxx \
  test/test37.cxx \
  test/test39.cxx \
  test/test46.cxx \
  test/test56.cxx \
  test/test60.cxx \
  test/test61.cxx \
  test/test62.cxx \
  test/test70.cxx \
  test/test71.cxx \
  test/test72.cxx \
  test/test74.cxx \
  test/test75.cxx \
  test/test76.cxx \
  test/test77.cxx \
  test/test82.cxx \
  test/test84.cxx \
  test/test88.cxx \
  test/test89.cxx \
  test/test90.cxx \
  test/test_array.cxx \
  test/test_blob.cxx \
  test/test_cancel_query.cxx \
  test/test_column.cxx \
  test/test_composite.cxx \
  test/test_connection.cxx \
  test/test_connection_string.cxx \
  test/test_cursor.cxx \
  test/test_encodings.cxx \
  test/test_error_verbosity.cxx \
  test/test_errorhandler.cxx \
  test/test_escape.cxx \
  test/test_exceptions.cxx \
  test/test_field.cxx \
  test/test_float.cxx \
  test/test_helpers.cxx \
  test/test_largeobject.cxx \
  test/test_nonblocking_connect.cxx \
  test/test_notice_handler.cxx \
  test/test_notification.cxx \
  test/test_params.cxx \
  test/test_pipeline.cxx \
  test/test_prepared_statement.cxx \
  test/test_range.cxx \
  test/test_read_transaction.cxx \
  test/test_result_iteration.cxx \
  test/test_row.cxx \
  test/test_separated_list.cxx \
  test/test_simultaneous_transactions.cxx \
  test/test_sql_cursor.cxx \
  test/test_stateless_cursor.cxx \
  test/test_strconv.cxx \
  test/test_stream_from.cxx \
  test/test_stream_query.cxx \
  test/test_stream_to.cxx \
  test/test_string_conversion.cxx \
  test/test_subtransaction.cxx \
  test/test_thread_safety_model.cxx \
  test/test_time.cxx \
  test/test_transaction.cxx \
  test/test_transaction_base.cxx \
  test/test_transaction_focus.cxx \
  test/test_transactor.cxx \
  test/test_type_name.cxx \
  test/test_util.cxx \
  test/test_zview.cxx \
  test/runner.cxx

test_runner_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}


PQXX_EXAMPLES = \
  examples/complete_example \
  examples/faster_data \
  examples/from_readme \
  examples/getting_started_1 \
  examples/getting_started_2 \
  examples/quick_example \
  examples/simple_queries \
  # (End of list)

noinst_PROGRAMS = $(PQXX_EXAMPLES) tools/benchmark

examples_complete_example_SOURCES = examples/complete_example.cxx
examples_complete_example_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
examples_faster_data_SOURCES = examples/faster_data.cxx
examples_faster_data_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
examples_from_readme_SOURCES = examples/from_readme.cxx
examples_from_readme_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
examples_getting_started_1_SOURCES = examples/getting_started_1.cxx
examples_getting_started_1_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
examples_getting_started_2_SOURCES = examples/getting_started_2.cxx
examples_getting_started_2_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
examples_quick_example_SOURCES = examples/quick_example.cxx
examples_quick_example_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}
examples_simple_queries_SOURCES = examples/simple_queries.cxx
examples_simple_queries_LDADD = $(top_builddir)/src/libpqxx.la ${POSTGRES_LIB}

TESTS = $(PQXX_TEST_PROGS) tools/lint.sh $(PQXX_EXAMPLES)

check_PROGRAMS = ${PQXX_TEST_PROGS} tools/benchmark


maintainer-clean-local:
	$(RM) -rf doc/doxygen-html
	$(MKDIR) doc/doxygen-html

all-local: docs

if BUILD_DOCS
DOXYGEN = doxygen
# Doxygen config needs the source files in its local tree.  That means the
# build tree.  If that differs from the source tree, copy the files.
docs:
	if ! [ "$(top_srcdir)/src" -ef src ]; \
	then \
	    cp "$(top_srcdir)"/src/*.[ch]xx src/ ; \
	    cp -r "$(top_srcdir)"/include/pqxx/* include/pqxx/ ; \
	    cp -r "$(top_srcdir)"/test test/ ; \
	fi
	$(DOXYGEN) "$(top_srcdir)/doc/Doxyfile"
else
docs:
endif

dist-hook:
	if [ -d doc/doxygen-html ]; \
	then \
	    cp -pR doc/doxygen-html $(top_distdir)/doc/html ; \
	fi
