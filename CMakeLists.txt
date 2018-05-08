cmake_minimum_required (VERSION 3.0)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAK_CXX_EXTENSIONS OFF)

option(BUILD_TEST "Build all test cases" ON)

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
