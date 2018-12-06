if(NOT PostgreSQL_INCLUDE_DIRS)
        find_package(PostgreSQL REQUIRED)
endif()

macro(library_target_setup tgt)
    target_include_directories(${tgt}
        PUBLIC
            $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
            $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/include>
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

file(GLOB CXX_SOURCES "*.cxx")

if(NOT SKIP_PQXX_SHARED)
	# Build a shared library
	add_library(pqxx_shared SHARED ${CXX_SOURCES})
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

