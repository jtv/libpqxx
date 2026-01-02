file(GLOB SOURCE_FILES "*.cxx")

foreach(source_file ${SOURCE_FILES})
    get_filename_component(name ${source_file} NAME_WLE)
    add_executable(${name} ${source_file})
    target_link_libraries(${name} PRIVATE pqxx)
    add_test(NAME ${name} COMMAND ${name})
endforeach()
