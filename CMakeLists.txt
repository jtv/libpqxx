cmake_minimum_required (VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PostgreSQL REQUIRED)

file(GLOB CXX_SOURCES "src/*.cxx")

# Build a shared library
add_library(pqxx_shared SHARED ${CXX_SOURCES})
target_include_directories(pqxx_shared PUBLIC include config/sample-headers/compiler/gcc-7.2 ${PostgreSQL_INCLUDE_DIRS})
target_compile_definitions(pqxx_shared PUBLIC -DPQXX_SHARED)
target_link_libraries(pqxx_shared PUBLIC ${PostgreSQL_LIBRARIES})
set_target_properties(pqxx_shared PROPERTIES OUTPUT_NAME pqxx)

# Build a static libary
add_library(pqxx_static STATIC ${CXX_SOURCES})
target_include_directories(pqxx_static PUBLIC include config/sample-headers/compiler/gcc-7.2 ${PostgreSQL_INCLUDE_DIRS})
target_link_libraries(pqxx_static PUBLIC ${PostgreSQL_LIBRARIES})
set_target_properties(pqxx_static PROPERTIES OUTPUT_NAME pqxx)


# Build tests for the library
enable_testing()
file(GLOB TEST_SOURCES test/test*.cxx)
add_executable(runner
        test/runner.cxx
        ${TEST_SOURCES})
target_include_directories(runner PUBLIC include config/sample-headers/compiler/gcc-7.2)
target_link_libraries(runner PUBLIC pqxx_shared)
add_test(NAME runner WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND runner)
