function(detect_code_compiled code macro msg)
    message(STATUS "Detecting ${msg}")
    check_cxx_source_compiles("${code}" "${macro}" FAIL_REGEX "warning")
    if(${macro})
        message(STATUS "Detecting ${msg} - supported")
    else()
        message(STATUS "Detecting ${msg} - not supported")
    endif()
endfunction(detect_code_compiled)

include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckSymbolExists)
include(CMakeDetermineCompileFeatures)
include(CheckCXXSourceCompiles)
include(CMakeFindDependencyMacro)

if(NOT PostgreSQL_FOUND)
    if(POLICY CMP0074)
        cmake_policy(PUSH)
        # CMP0074 is `OLD` by `cmake_minimum_required(VERSION 3.7)`, sets `NEW`
        # to enable support CMake variable `PostgreSQL_ROOT`.
        cmake_policy(SET CMP0074 NEW)
    endif()

    find_package(PostgreSQL)

    if(POLICY CMP0074)
        cmake_policy(POP)
    endif()
endif()

if(NOT PostgreSQL_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PostgreSQL REQUIRED libpq)
endif()

check_function_exists("poll" PQXX_HAVE_POLL)

set(CMAKE_REQUIRED_LIBRARIES pq)

cmake_determine_compile_features(CXX)
cmake_policy(SET CMP0057 NEW)

# check_cxx_source_compiles requires CMAKE_REQUIRED_DEFINITIONS to specify
# compiling arguments. Workaround: Push CMAKE_REQUIRED_DEFINITIONS
if(CMAKE_REQUIRED_DEFINITIONS)
    set(def "${CMAKE_REQUIRED_DEFINITIONS}")
endif()
set(CMAKE_REQUIRED_DEFINITIONS
    ${CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION}
)
set(CMAKE_REQUIRED_QUIET ON)

# Incorporate feature checks based on C++ feature test mac
include(pqxx_cxx_feature_checks)

# This variable is set by one of the snippets in config-tests.
if(!no_need_fslib)
    # TODO: This may work for gcc 8, but some clang versions may need -lc++fs.
    link_libraries(stdc++fs)
endif()

# check_cxx_source_compiles requires CMAKE_REQUIRED_DEFINITIONS to specify
# compiling arguments. Workaround: Pop CMAKE_REQUIRED_DEFINITIONS
if(def)
    set(CMAKE_REQUIRED_DEFINITIONS ${def})
    unset(def CACHE)
else()
    unset(CMAKE_REQUIRED_DEFINITIONS CACHE)
endif()
set(CMAKE_REQUIRED_QUIET OFF)

set(AC_CONFIG_H_IN "${PROJECT_SOURCE_DIR}/include/pqxx/config.h.in")
set(CM_CONFIG_H_IN "${PROJECT_BINARY_DIR}/include/pqxx/config_cmake.h.in")
set(CM_CONFIG_PUB "${PROJECT_BINARY_DIR}/include/pqxx/config-public-compiler.h")
set(CM_CONFIG_INT
    "${PROJECT_BINARY_DIR}/include/pqxx/config-internal-compiler.h"
)
set(CM_CONFIG_PQ "${PROJECT_BINARY_DIR}/include/pqxx/config-internal-libpq.h")
message(STATUS "Generating config.h")
file(WRITE "${CM_CONFIG_H_IN}" "")
file(STRINGS "${AC_CONFIG_H_IN}" lines)
foreach(line ${lines})
    string(REGEX REPLACE "^#undef" "#cmakedefine" l "${line}")
    file(APPEND "${CM_CONFIG_H_IN}" "${l}\n")
endforeach()
configure_file("${CM_CONFIG_H_IN}" "${CM_CONFIG_INT}" @ONLY)
configure_file("${CM_CONFIG_H_IN}" "${CM_CONFIG_PUB}" @ONLY)
configure_file("${CM_CONFIG_H_IN}" "${CM_CONFIG_PQ}" @ONLY)
message(STATUS "Generating config.h - done")
