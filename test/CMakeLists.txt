################################################################################
# AUTOMATICALLY GENERATED FILE -- DO NOT EDIT.
#
# This file is generated automatically by libpqxx's template2mak.py script, and
# will be rewritten from time to time.
#
# If you modify this file, chances are your modifications will be lost.
#
# The template2mak.py script should be available in the tools directory of the
# libpqxx source archive.
#
# Generated from template './test/CMakeLists.txt.template'.
################################################################################
enable_testing()

# TODO: We're giving exact filenames here.  Don't pretend they're globs.
file(
    GLOB TEST_SOURCES
    test00.cxx
    test01.cxx
    test02.cxx
    test04.cxx
    test07.cxx
    test10.cxx
    test11.cxx
    test12.cxx
    test13.cxx
    test14.cxx
    test15.cxx
    test16.cxx
    test17.cxx
    test18.cxx
    test20.cxx
    test21.cxx
    test23.cxx
    test26.cxx
    test29.cxx
    test30.cxx
    test31.cxx
    test32.cxx
    test33.cxx
    test34.cxx
    test35.cxx
    test36.cxx
    test37.cxx
    test39.cxx
    test46.cxx
    test48.cxx
    test49.cxx
    test50.cxx
    test51.cxx
    test52.cxx
    test53.cxx
    test54.cxx
    test55.cxx
    test56.cxx
    test57.cxx
    test58.cxx
    test59.cxx
    test60.cxx
    test61.cxx
    test62.cxx
    test63.cxx
    test64.cxx
    test65.cxx
    test66.cxx
    test67.cxx
    test69.cxx
    test70.cxx
    test71.cxx
    test72.cxx
    test73.cxx
    test74.cxx
    test75.cxx
    test76.cxx
    test77.cxx
    test78.cxx
    test79.cxx
    test82.cxx
    test84.cxx
    test86.cxx
    test87.cxx
    test88.cxx
    test89.cxx
    test90.cxx
    test92.cxx
    test93.cxx
    runner.cxx
)

add_executable(runner ${TEST_SOURCES})
target_link_libraries(runner PUBLIC pqxx_shared)
add_test(NAME runner WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND runner)

add_subdirectory(unit)
