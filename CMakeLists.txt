cmake_minimum_required(VERSION 3.12)
file(READ VERSION VER_FILE_CONTENT)
string(STRIP ${VER_FILE_CONTENT} VER_FILE_CONTENT)

# Remove any release-candidate suffix.
string(REGEX MATCH "^[0-9.]+" STRIPPED_VERSION ${VER_FILE_CONTENT})
project(
    libpqxx
    VERSION "${STRIPPED_VERSION}"
    LANGUAGES CXX
)
if(NOT "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 20)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Generate a "compilation database" so we can use run-clang-tidy, to run static
# checks in parallel.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(BUILD_DOC "Build documentation" OFF)

if(NOT SKIP_BUILD_TEST AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    option(BUILD_TEST "Build all test cases" ON)
endif()

# TODO: Enable PQXX_HAVE_STACKTRACE if appropriate.

if(NOT SKIP_BUILD_EXAMPLES AND
   CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR
)
    option(BUILD_EXAMPLES "Build examples" ON)
endif()
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(config)
add_subdirectory(src)
add_subdirectory(include)
if(BUILD_DOC)
    add_subdirectory(doc)
endif()
if(BUILD_TEST)
    add_subdirectory(test)
endif()
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Installation
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libpqxx-config-version.cmake"
    VERSION ${VER_FILE_CONTENT}
    COMPATIBILITY SameMajorVersion
)
install(FILES cmake/libpqxx-config.cmake
              "${CMAKE_CURRENT_BINARY_DIR}/libpqxx-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpqxx
)
install(
    EXPORT libpqxx-targets
    NAMESPACE libpqxx::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libpqxx
)
# Build tree export
export(
    EXPORT libpqxx-targets
    NAMESPACE libpqxx::
    FILE ${CMAKE_CURRENT_BINARY_DIR}/libpqxx-targets.cmake
)
configure_file(
    cmake/libpqxx-config.cmake ${CMAKE_CURRENT_BINARY_DIR}/libpqxx-config.cmake
    COPYONLY
)
# Package generation
set(CPACK_GENERATOR TGZ)
set(CPACK_PACKAGE_VERSION ${VER_FILE_CONTENT})
include(CPack)
