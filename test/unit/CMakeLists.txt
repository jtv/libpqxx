file(GLOB UNIT_TEST_SOURCES *.cxx)

add_executable(unit_runner ${UNIT_TEST_SOURCES})
target_link_libraries(unit_runner PUBLIC pqxx_shared)
add_test(NAME unit_runner WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND unit_runner)
