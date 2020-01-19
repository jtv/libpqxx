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
# Generated from template './doc/CMakeLists.txt.template'.
################################################################################
find_program(HAVE_DOXYGEN doxygen)

if(NOT HAVE_DOXYGEN)
    message(FATAL_ERROR "*****************************************************
Doxygen not found.
Install it, or configure with -DBUILD_DOC=OFF
*****************************************************")
endif()

find_program(HAVE_XMLTO xmlto)

if(NOT HAVE_XMLTO)
    message(FATAL_ERROR "*****************************************************
xmlto not found.
Install it, or configure with -DBUILD_DOC=OFF
*****************************************************")
endif()

set(PQXXVERSION "${CMAKE_PROJECT_VERSION}")
set(top_srcdir "${PROJECT_SOURCE_DIR}")
set(PQXX_ABI "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(PQXX_MAJOR "${PROJECT_VERSION_MAJOR}")
set(PQXX_MINOR "${PROJECT_VERSION_MINOR}")

find_program(HAVE_DOT dot)
if(HAVE_DOT)
    set(HAVE_DOT YES)
else()
    set(HAVE_DOT NO)
endif()

configure_file(Doxyfile.in Doxyfile)

if(HAVE_DOXYGEN)
    set(DOXYGEN_SOURCES
    	"${PROJECT_SOURCE_DIR}/include/pqxx/array.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/binarystring.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/compiler-public.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/connection.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/cursor.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/dbtransaction.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/errorhandler.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/except.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/field.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/isolation.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/largeobject.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/nontransaction.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/notification.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/pipeline.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/prepared_statement.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/result.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/result_iterator.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/robusttransaction.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/row.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/separated_list.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/strconv.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/stream_from.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/stream_to.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/subtransaction.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/transaction.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/transaction_base.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/transactor.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/types.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/util.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/version.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/zview.hxx"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/README.md"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/accessing-results.md"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/escaping.md"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/getting-started.md"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/mainpage.md"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/performance.md"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/prepared-statement.md"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/streams.md"
    	"${PROJECT_SOURCE_DIR}/include/pqxx/doc/thread-safety.md"
    	"${PROJECT_SOURCE_DIR}/src/array.cxx"
    	"${PROJECT_SOURCE_DIR}/src/binarystring.cxx"
    	"${PROJECT_SOURCE_DIR}/src/connection.cxx"
    	"${PROJECT_SOURCE_DIR}/src/cursor.cxx"
    	"${PROJECT_SOURCE_DIR}/src/encodings.cxx"
    	"${PROJECT_SOURCE_DIR}/src/errorhandler.cxx"
    	"${PROJECT_SOURCE_DIR}/src/except.cxx"
    	"${PROJECT_SOURCE_DIR}/src/field.cxx"
    	"${PROJECT_SOURCE_DIR}/src/largeobject.cxx"
    	"${PROJECT_SOURCE_DIR}/src/notification.cxx"
    	"${PROJECT_SOURCE_DIR}/src/pipeline.cxx"
    	"${PROJECT_SOURCE_DIR}/src/result.cxx"
    	"${PROJECT_SOURCE_DIR}/src/robusttransaction.cxx"
    	"${PROJECT_SOURCE_DIR}/src/row.cxx"
    	"${PROJECT_SOURCE_DIR}/src/sql_cursor.cxx"
    	"${PROJECT_SOURCE_DIR}/src/statement_parameters.cxx"
    	"${PROJECT_SOURCE_DIR}/src/strconv.cxx"
    	"${PROJECT_SOURCE_DIR}/src/stream_from.cxx"
    	"${PROJECT_SOURCE_DIR}/src/stream_to.cxx"
    	"${PROJECT_SOURCE_DIR}/src/subtransaction.cxx"
    	"${PROJECT_SOURCE_DIR}/src/transaction.cxx"
    	"${PROJECT_SOURCE_DIR}/src/transaction_base.cxx"
    	"${PROJECT_SOURCE_DIR}/src/util.cxx"
    	"${PROJECT_SOURCE_DIR}/src/version.cxx"
    )
    set(DOXYGEN_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/doxygen.stamp")
    add_custom_command(OUTPUT ${DOXYGEN_STAMP_FILE}
    	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/html/Reference
    	COMMAND doxygen Doxyfile
    	COMMAND ${CMAKE_COMMAND} -E touch ${DOXYGEN_STAMP_FILE}
    	DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${DOXYGEN_SOURCES}
    	COMMENT "Generate API documentation"
	VERBATIM
    )
    add_custom_target(doxygen ALL
    	DEPENDS ${DOXYGEN_STAMP_FILE}
    	SOURCES ${DOXYGEN_SOURCES}
    )
    install(
    	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/Reference
    	DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
    )
endif()

if(HAVE_XMLTO)
    set(XMLTO_SOURCES
    	"${CMAKE_CURRENT_SOURCE_DIR}/libpqxx.xml"
    )
    set(XMLTO_STAMP_FILE "${CMAKE_CURRENT_BINARY_DIR}/xmlto.stamp")
    add_custom_command(OUTPUT ${XMLTO_STAMP_FILE}
    	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/html/Tutorial
    	COMMAND xmlto -o html/Tutorial xhtml "${CMAKE_CURRENT_SOURCE_DIR}/libpqxx.xml"
    	COMMAND ${CMAKE_COMMAND} -E touch ${XMLTO_STAMP_FILE}
    	DEPENDS ${XMLTO_SOURCES}
    	COMMENT "Generate tutorial"
	VERBATIM
    )
    add_custom_target(tutorial ALL
    	DEPENDS ${XMLTO_STAMP_FILE}
    	SOURCES ${XMLTO_SOURCES}
    )
    install(
    	DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html/Tutorial
    	DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
    )
endif()
