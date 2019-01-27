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
# Generated from template './test/unit/CMakeLists.txt.template'.
################################################################################
if(NOT PostgreSQL_INCLUDE_DIRS)
    find_package(PostgreSQL REQUIRED)
endif()

# TODO: We're giving exact filenames here.  Don't pretend they're globs.
file(
    GLOB UNIT_TEST_SOURCES
    runner.cxx
    test_array.cxx
    test_binarystring.cxx
    test_cancel_query.cxx
    test_cursor.cxx
    test_encodings.cxx
    test_error_verbosity.cxx
    test_errorhandler.cxx
    test_escape.cxx
    test_exceptions.cxx
    test_float.cxx
    test_notification.cxx
    test_parameterized.cxx
    test_pipeline.cxx
    test_prepared_statement.cxx
    test_read_transaction.cxx
    test_result_iteration.cxx
    test_result_slicing.cxx
    test_row.cxx
    test_simultaneous_transactions.cxx
    test_sql_cursor.cxx
    test_stateless_cursor.cxx
    test_stream_from.cxx
    test_stream_to.cxx
    test_string_conversion.cxx
    test_subtransaction.cxx
    test_test_helpers.cxx
    test_thread_safety_model.cxx
    test_transaction_base.cxx
    test_transactor.cxx
)

add_executable(unit_runner ${UNIT_TEST_SOURCES})
target_link_libraries(unit_runner PUBLIC pqxx_shared)
target_include_directories(unit_runner PRIVATE ${PostgreSQL_INCLUDE_DIRS})
add_test(
    NAME unit_runner
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMAND unit_runner)
