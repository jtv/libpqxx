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
set(CONFIG_H_COM "${PROJECT_BINARY_DIR}/include/pqxx/config-compiler.h")
message(STATUS "Generating configuration headers")

# First we write config_cmake.h.in based on autoconf's config.h.in.
file(WRITE "${CM_CONFIG_H_IN}" "")
file(STRINGS "${AC_CONFIG_H_IN}" lines)
foreach(line ${lines})
    string(REGEX REPLACE "^#undef" "#cmakedefine" l "${line}")
    file(APPEND "${CM_CONFIG_H_IN}" "${l}\n")
endforeach()

# Now have CMake write config.h based on that config_cmake.h.in.  This makes the
# process look as much like the autoconf one as we can.
configure_file("${CM_CONFIG_H_IN}" "${CONFIG_H}" @ONLY)

# Then grab the PQXX macros from config.h and write them to config-compiler.h.
execute_process(
    COMMAND grep PQXX "${CONFIG_H}"
    OUTPUT_FILE "${CONFIG_H_COM}"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
message(STATUS "Generating configuration headers - done")
