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
