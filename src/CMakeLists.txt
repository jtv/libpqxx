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
# Generated from template './src/CMakeLists.txt.template'.
################################################################################
if(NOT PostgreSQL_INCLUDE_DIRS)
        find_package(PostgreSQL REQUIRED)
endif()

# When setting up the include paths, mention the binary tree's include
# directory *before* the source tree's include directory.  If the source tree
# happens to contain autoconf-generated config headers, we should still prefer
# the ones in the binary tree.
macro(library_target_setup tgt)
    target_include_directories(${tgt}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE
            ${PostgreSQL_INCLUDE_DIRS}
    )
    target_link_libraries(${tgt} PRIVATE ${PostgreSQL_LIBRARIES})
    if(WIN32)
        target_link_libraries(${tgt} PUBLIC wsock32 ws2_32)
    endif()
    install(TARGETS ${tgt} EXPORT libpqxx-targets
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endmacro()

# TODO: We're giving exact filenames here.  Don't pretend they're globs.
file(
    GLOB CXX_SOURCES
    array.cxx
    binarystring.cxx
    connection.cxx
    connection_base.cxx
    cursor.cxx
    dbtransaction.cxx
    encodings.cxx
    errorhandler.cxx
    except.cxx
    field.cxx
    largeobject.cxx
    nontransaction.cxx
    notification.cxx
    pipeline.cxx
    prepared_statement.cxx
    result.cxx
    robusttransaction.cxx
    row.cxx
    sql_cursor.cxx
    statement_parameters.cxx
    strconv.cxx
    stream_base.cxx
    stream_from.cxx
    stream_to.cxx
    subtransaction.cxx
    tablereader.cxx
    tablestream.cxx
    tablewriter.cxx
    transaction.cxx
    transaction_base.cxx
    util.cxx
    version.cxx
)

if(NOT SKIP_PQXX_SHARED)
    set(CXX_SHARED_EXTRA_SOURCE)
    if(WIN32)
        # Add extra source
        list(APPEND CXX_SHARED_EXTRA_SOURCE "${PROJECT_SOURCE_DIR}/win32/libpqxx.cxx")
    endif()

	# Build a shared library
	add_library(pqxx_shared SHARED ${CXX_SOURCES} ${CXX_SHARED_EXTRA_SOURCE})
	target_compile_definitions(pqxx_shared PUBLIC -DPQXX_SHARED)
	set_target_properties(pqxx_shared PROPERTIES OUTPUT_NAME pqxx)
	library_target_setup(pqxx_shared)
endif()

if(NOT SKIP_PQXX_STATIC)
	# Build a static libary
	add_library(pqxx_static STATIC ${CXX_SOURCES})
	set_target_properties(pqxx_static PROPERTIES OUTPUT_NAME pqxx)
	library_target_setup(pqxx_static)
endif()
