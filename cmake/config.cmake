include(CheckFunctionExists)
include(CMakeFindDependencyMacro)
if(NOT PostgreSQL_FOUND)
    find_package(PostgreSQL)
endif()
if(NOT PostgreSQL_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PostgreSQL REQUIRED libpq)
endif()

# Run our generated config checks.
#
# Some of these may require libpq.  Temporarily set CMAKE_REQUIRED_INCLUDES
# and CMAKE_REQUIRED_LIBRARIES so that they can find the libpq headers and
# binary.
set(CMAKE_REQUIRED_INCLUDES ${PostgreSQL_INCLUDE_DIRS})
set(CMAKE_REQUIRED_LIBRARIES PostgreSQL::PostgreSQL)

# However we'll skip linking; we only need that for one of the checks so we
# can save some time doing it only incidentally.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Incorporate feature checks we generated from the config-tests directory.
include(pqxx_cxx_feature_checks)

# Now restore previous settings.
unset(CMAKE_TRY_COMPILE_TARGET_TYPE)
unset(CMAKE_REQUIRED_LIBRARIES)
unset(CMAKE_REQUIRED_INCLUDES)

# TODO: Separate linking/compile-only checks somehow.

# Check for std::stacktrace support.  We can't just generate this like other
# tests because some compilers require an extra library to support
# std::stacktrace.  We need to link as well, not just compile.
try_compile(
    PQXX_HAVE_STACKTRACE ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/stacktrace_support.cxx
)

set(AC_CONFIG_H_IN "${PROJECT_SOURCE_DIR}/include/pqxx/internal/config.h.in")
set(CM_CONFIG_H_IN
    "${PROJECT_BINARY_DIR}/include/pqxx/internal/config_cmake.h.in"
)
set(CONFIG_H "${PROJECT_BINARY_DIR}/include/pqxx/internal/config.h")
message(STATUS "Generating configuration headers")

# First we write config_cmake.h.in based on autoconf's config.h.in.
file(WRITE "${CM_CONFIG_H_IN}" "")
file(STRINGS "${AC_CONFIG_H_IN}" lines)
foreach(line ${lines})
    string(REGEX REPLACE "^#undef" "#cmakedefine" l "${line}")
    file(APPEND "${CM_CONFIG_H_IN}" "${l}\n")
endforeach()

# Now have CMake write config.h based on that config_cmake.h.in.  This makes
# the process look as much like the autoconf one as we can.
configure_file("${CM_CONFIG_H_IN}" "${CONFIG_H}" @ONLY)

message(STATUS "Generating configuration headers - done")
