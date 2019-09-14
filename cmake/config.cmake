function(detect_code_compiled code macro msg)
  message(STATUS "Detecting ${msg}")
  check_cxx_source_compiles("${code}" "${macro}" FAIL_REGEX "warning")
  if (${macro})
    message(STATUS "Detecting ${msg} - supported")
  else (${macro})
    message(STATUS "Detecting ${msg} - not supported")
  endif (${macro})
endfunction(detect_code_compiled)

include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CMakeDetermineCompileFeatures)
include(CheckCXXSourceCompiles)

check_function_exists("poll" PQXX_HAVE_POLL)

cmake_determine_compile_features(CXX)
cmake_policy(SET CMP0057 NEW)
if (cxx_attribute_deprecated IN_LIST CMAKE_CXX_COMPILE_FEATURES)
  set(PQXX_HAVE_DEPRECATED)
endif ()
# detect_cxx_feature("cxx_attribute_deprecated" "PQXX_HAVE_DEPRECATED")

# check_cxx_source_compiles requires CMAKE_REQUIRED_DEFINITIONS to specify
# compiling arguments.
# Wordaround: Push CMAKE_REQUIRED_DEFINITIONS
if (CMAKE_REQUIRED_DEFINITIONS)
  set(def "${CMAKE_REQUIRED_DEFINITIONS}")
endif (CMAKE_REQUIRED_DEFINITIONS)
set(CMAKE_REQUIRED_DEFINITIONS ${CMAKE_CXX${CMAKE_CXX_STANDARD}_STANDARD_COMPILE_OPTION})
set(CMAKE_REQUIRED_QUIET ON)

try_compile(PQXX_HAVE_GCC_PURE ${PROJECT_BINARY_DIR} SOURCES ${PROJECT_SOURCE_DIR}/config-tests/gcc_pure.cxx)
try_compile(PQXX_HAVE_GCC_VISIBILITY ${PROJECT_BINARY_DIR} SOURCES ${PROJECT_SOURCE_DIR}/config-tests/gcc_visibility.cxx)
try_compile(PQXX_HAVE_CXA_DEMANGLE ${PROJECT_BINARY_DIR} SOURCES ${PROJECT_SOURCE_DIR}/config-tests/cxa_demangle.cxx)
try_compile(PQXX_HAVE_CHARCONV_FLOAT ${PROJECT_BINARY_DIR} SOURCES ${PROJECT_SOURCE_DIR}/config-tests/charconv_float.cxx)
try_compile(PQXX_HAVE_CHARCONV_INT ${PROJECT_BINARY_DIR} SOURCES ${PROJECT_SOURCE_DIR}/config-tests/charconv_int.cxx)

# check_cxx_source_compiles requires CMAKE_REQUIRED_DEFINITIONS to specify
# compiling arguments.
# Wordaround: Pop CMAKE_REQUIRED_DEFINITIONS
if (def)
  set(CMAKE_REQUIRED_DEFINITIONS ${def})
  unset(def CACHE)
else (def)
  unset(CMAKE_REQUIRED_DEFINITIONS CACHE)
endif (def)
set(CMAKE_REQUIRED_QUIET OFF)

set(AC_CONFIG_H_IN "${PROJECT_SOURCE_DIR}/include/pqxx/config.h.in")
set(CM_CONFIG_H_IN "${PROJECT_BINARY_DIR}/include/pqxx/config_cmake.h.in")
set(CM_CONFIG_PUB "${PROJECT_BINARY_DIR}/include/pqxx/config-public-compiler.h")
set(CM_CONFIG_INT "${PROJECT_BINARY_DIR}/include/pqxx/config-internal-compiler.h")
message(STATUS "Generating config.h")
file(WRITE "${CM_CONFIG_H_IN}" "")
file(STRINGS "${AC_CONFIG_H_IN}" lines)
foreach (line ${lines})
  string(REGEX REPLACE "^#undef" "#cmakedefine" l "${line}")
  file(APPEND "${CM_CONFIG_H_IN}" "${l}\n")
endforeach (line)
configure_file("${CM_CONFIG_H_IN}" "${CM_CONFIG_INT}" @ONLY)
configure_file("${CM_CONFIG_H_IN}" "${CM_CONFIG_PUB}" @ONLY)
message(STATUS "Generating config.h - done")
