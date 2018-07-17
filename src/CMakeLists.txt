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

# Build a static libary
add_library(pqxx_static STATIC ${CXX_SOURCES})
set_target_properties(pqxx_static PROPERTIES OUTPUT_NAME pqxx)
target_include_directories(pqxx_static PUBLIC ${INCLUDE_DIRS})
