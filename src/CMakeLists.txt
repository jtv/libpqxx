file(GLOB CXX_SOURCES "*.cxx")

set(INCLUDE_DIRS
	${CMAKE_CURRENT_SOURCE_DIR}/../include
	${CMAKE_CURRENT_BINARY_DIR}/../include
	)

# Build a shared library
add_library(pqxx_shared SHARED ${CXX_SOURCES})
target_compile_definitions(pqxx_shared PUBLIC -DPQXX_SHARED)
set_target_properties(pqxx_shared PROPERTIES OUTPUT_NAME pqxx)
target_include_directories(pqxx_shared PUBLIC ${INCLUDE_DIRS})
if (WIN32)
	target_link_libraries(pqxx_shared
		PUBLIC
			wsock32
	)
endif ()

# Build a static libary
add_library(pqxx_static STATIC ${CXX_SOURCES})
set_target_properties(pqxx_static PROPERTIES OUTPUT_NAME pqxx)
target_include_directories(pqxx_static PUBLIC ${INCLUDE_DIRS})
if (WIN32)
	target_compile_options(pqxx_static
		PRIVATE
			$<$<CXX_COMPILER_ID:MSVC>:
				/MP /Zf /W3
				$<$<CONFIG:Debug>:/MTd>
				$<$<CONFIG:Release>:/MT>>
	)
endif ()
