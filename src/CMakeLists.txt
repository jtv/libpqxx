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
    get_target_property(name ${tgt} NAME)
    get_target_property(output_name ${tgt} OUTPUT_NAME)
    if(NOT name STREQUAL output_name)
        # Create library symlink
        get_target_property(target_type ${tgt} TYPE)
        if(target_type STREQUAL "SHARED_LIBRARY")
            set(library_prefix ${CMAKE_SHARED_LIBRARY_PREFIX})
            set(library_suffix ${CMAKE_SHARED_LIBRARY_SUFFIX})
        elseif(target_type STREQUAL "STATIC_LIBRARY")
            set(library_prefix ${CMAKE_STATIC_LIBRARY_PREFIX})
            set(library_suffix ${CMAKE_STATIC_LIBRARY_SUFFIX})
        endif()
        add_custom_command(TARGET ${tgt} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E create_symlink "${library_prefix}${output_name}${library_suffix}" "${library_prefix}${name}${library_suffix}"
            VERBATIM
            COMMAND_EXPAND_LISTS
        )
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${library_prefix}${name}${library_suffix}
        	DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
    endif()
endmacro()

file(
	GLOB CXX_SOURCES
	array.cxx
	binarystring.cxx
	connection.cxx
	cursor.cxx
	encodings.cxx
	errorhandler.cxx
	except.cxx
	field.cxx
	largeobject.cxx
	notification.cxx
	pipeline.cxx
	result.cxx
	robusttransaction.cxx
	row.cxx
	sql_cursor.cxx
	statement_parameters.cxx
	strconv.cxx
	stream_from.cxx
	stream_to.cxx
	subtransaction.cxx
	transaction.cxx
	transaction_base.cxx
	util.cxx
	version.cxx
)

add_library(pqxx ${CXX_SOURCES})

get_target_property(pqxx_target_type pqxx TYPE)
if(pqxx_target_type STREQUAL "SHARED_LIBRARY")
    target_compile_definitions(pqxx PUBLIC PQXX_SHARED)
endif()

set_target_properties(
	pqxx PROPERTIES
	OUTPUT_NAME pqxx-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)
library_target_setup(pqxx)

# install pkg-config file
set(prefix ${CMAKE_INSTALL_PREFIX})
set(exec_prefix \${prefix})
set(libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
set(VERSION ${PROJECT_VERSION})
configure_file(${PROJECT_SOURCE_DIR}/libpqxx.pc.in ${PROJECT_BINARY_DIR}/libpqxx.pc)
install(FILES ${PROJECT_BINARY_DIR}/libpqxx.pc
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
