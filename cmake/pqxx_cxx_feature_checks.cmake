# Configuration for feature checks. Generated by generate_check_config.py.
try_compile(
    PQXX_HAVE_ASSUME ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_ASSUME.cxx
)
try_compile(
    PQXX_HAVE_CHARCONV_FLOAT ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_CHARCONV_FLOAT.cxx
)
try_compile(
    PQXX_HAVE_CXA_DEMANGLE ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_CXA_DEMANGLE.cxx
)
try_compile(
    PQXX_HAVE_GCC_PURE ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_GCC_PURE.cxx
)
try_compile(
    PQXX_HAVE_GCC_VISIBILITY ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_GCC_VISIBILITY.cxx
)
try_compile(
    PQXX_HAVE_MULTIDIM ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_MULTIDIM.cxx
)
try_compile(
    PQXX_HAVE_POLL ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_POLL.cxx
)
try_compile(
    PQXX_HAVE_SLEEP_FOR ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_SLEEP_FOR.cxx
)
try_compile(
    PQXX_HAVE_STRERROR_R ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_STRERROR_R.cxx
)
try_compile(
    PQXX_HAVE_STRERROR_S ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_STRERROR_S.cxx
)
try_compile(
    PQXX_HAVE_THREAD_LOCAL ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_THREAD_LOCAL.cxx
)
try_compile(
    PQXX_HAVE_YEAR_MONTH_DAY ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/PQXX_HAVE_YEAR_MONTH_DAY.cxx
)
try_compile(
    no_need_fslib ${PROJECT_BINARY_DIR}
    SOURCES ${PROJECT_SOURCE_DIR}/config-tests/no_need_fslib.cxx
)
# End of config.
