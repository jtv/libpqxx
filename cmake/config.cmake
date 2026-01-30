include(CheckFunctionExists)
include(CMakeFindDependencyMacro)
if(NOT PostgreSQL_FOUND)
    find_package(PostgreSQL)
endif()
if(NOT PostgreSQL_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PostgreSQL REQUIRED libpq)
endif()
check_function_exists("poll" PQXX_HAVE_POLL)

# Incorporate feature checks based on C++ feature test macros.
include(pqxx_cxx_feature_checks)

# Check for std::stacktrace support.  We can't just generate this like other
# tests because some compilers require an extra library to support
# std::stacktrace.
try_compile(
    PQXX_HAVE_STACKTRACE ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/stacktrace_support.cxx
)

set(AC_CONFIG_H_IN "${PROJECT_SOURCE_DIR}/include/pqxx/config.h.in")
set(CM_CONFIG_H_IN "${PROJECT_BINARY_DIR}/include/pqxx/config_cmake.h.in")
set(CONFIG_H "${PROJECT_BINARY_DIR}/include/pqxx/config.h")
message(STATUS "Generating configuration headers")
file(WRITE "${CONFIG_H}" "")
file(STRINGS "${CONFIG_H_IN}" lines)
# TODO: Synthesise the autotools header ourselves?
foreach(line ${lines})
    string(REGEX REPLACE "^#undef" "#cmakedefine" l "${line}")
    file(APPEND "${CM_CONFIG_H_IN}" "${l}\n")
endforeach()
configure_file("${CM_CONFIG_H_IN}" "${CONFIG_H}" @ONLY)
execute_process(
    COMMAND ${CMAKE_COMMAND} -E env python3
            ${CMAKE_SOURCE_DIR}/tools/splitconfig.py ${CMAKE_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    RESULT_VARIABLE split_result
)
if(NOT split_result EQUAL 0)
    message(FATAL_ERROR "Could not split config headers.")
endif()
message(STATUS "Generating configuration headers - done")
