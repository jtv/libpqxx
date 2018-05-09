cmake_minimum_required (VERSION 3.0)
project(libpqxx)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAK_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

option(BUILD_TEST "Build all test cases" ON)

include(config)

find_package(PostgreSQL REQUIRED)
include_directories(
  ${CMAKE_SOURCE_DIR}/include
  config/sample-headers/compiler/gcc-7.2
  ${PostgreSQL_INCLUDE_DIRS}
)
link_libraries(${PostgreSQL_LIBRARIES})

add_subdirectory(src)
if (BUILD_TEST)
  add_subdirectory(test)
endif ()
