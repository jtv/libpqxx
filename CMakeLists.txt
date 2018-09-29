cmake_minimum_required (VERSION 3.7)
project(libpqxx)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if(NOT SKIP_BUILD_TEST)
	option(BUILD_TEST "Build all test cases" ON)
endif()

include(config)

if(NOT PostgreSQL_INCLUDE_DIRS)
	find_package(PostgreSQL REQUIRED)
endif()

include_directories(
  ${PROJECT_SOURCE_DIR}/include
  ${PostgreSQL_INCLUDE_DIRS}
)
link_libraries(${PostgreSQL_LIBRARIES})

add_subdirectory(src)
if (BUILD_TEST)
  add_subdirectory(test)
endif ()
