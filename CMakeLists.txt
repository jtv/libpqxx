cmake_minimum_required (VERSION 3.7)
project(libpqxx)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAK_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(BUILD_TEST "Build all test cases" ON)

include(config)

find_package(PostgreSQL REQUIRED)
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${PostgreSQL_INCLUDE_DIRS}
)
link_libraries(${PostgreSQL_LIBRARIES})

add_subdirectory(src)
if (BUILD_TEST)
  add_subdirectory(test)
endif ()
