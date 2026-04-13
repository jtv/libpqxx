if(NOT PostgreSQL_FOUND)
    find_package(PostgreSQL REQUIRED)
endif()
add_executable(benchmark benchmark.cxx)
target_link_libraries(benchmark PUBLIC pqxx PostgreSQL::PostgreSQL)
target_include_directories(benchmark PRIVATE ${PostgreSQL_INCLUDE_DIRS})
if(INSTALL_TOOLS)
    install(
        PROGRAMS benchmark
        TYPE BIN
        RENAME libpqxx-benchmark
    )
endif()
