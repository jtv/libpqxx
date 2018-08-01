enable_testing()

file(GLOB TEST_SOURCES test*.cxx runner.cxx)

add_executable(runner ${TEST_SOURCES})
target_link_libraries(runner PUBLIC pqxx_shared)
add_test(NAME runner WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND runner)

add_subdirectory(unit)
