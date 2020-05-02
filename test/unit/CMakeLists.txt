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
# Generated from template './test/unit/CMakeLists.txt.template'.
# ##############################################################################
if(NOT PostgreSQL_FOUND)
    if(POLICY CMP0074)
        cmake_policy(PUSH)
        # CMP0074 is `OLD` by `cmake_minimum_required(VERSION 3.7)`, sets `NEW`
        # to enable support CMake variable `PostgreSQL_ROOT`.
        cmake_policy(SET CMP0074 NEW)
    endif()

    find_package(PostgreSQL REQUIRED)

    if(POLICY CMP0074)
        cmake_policy(POP)
    endif()
endif()

file(
    GLOB
    UNIT_TEST_SOURCES
    test_array.cxx
    test_binarystring.cxx
    test_cancel_query.cxx
    test_column.cxx
    test_connection.cxx
    test_cursor.cxx
    test_encodings.cxx
    test_error_verbosity.cxx
    test_errorhandler.cxx
    test_escape.cxx
    test_exceptions.cxx
    test_field.cxx
    test_float.cxx
    test_largeobject.cxx
    test_notification.cxx
    test_pipeline.cxx
    test_prepared_statement.cxx
    test_read_transaction.cxx
    test_result_iteration.cxx
    test_result_slicing.cxx
    test_row.cxx
    test_separated_list.cxx
    test_simultaneous_transactions.cxx
    test_sql_cursor.cxx
    test_stateless_cursor.cxx
    test_strconv.cxx
    test_stream_from.cxx
    test_stream_to.cxx
    test_string_conversion.cxx
    test_subtransaction.cxx
    test_test_helpers.cxx
    test_thread_safety_model.cxx
    test_transaction.cxx
    test_transaction_base.cxx
    test_transactor.cxx
    test_type_name.cxx
)

add_executable(unit_runner ${UNIT_TEST_SOURCES})
target_link_libraries(unit_runner PUBLIC pqxx)
target_include_directories(unit_runner PRIVATE ${PostgreSQL_INCLUDE_DIRS})
add_test(
    NAME unit_runner
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMAND unit_runner
)
