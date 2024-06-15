find_program(HAVE_DOXYGEN doxygen)

set(PQXXVERSION "${CMAKE_PROJECT_VERSION}")
set(top_srcdir "${PROJECT_SOURCE_DIR}")
set(PQXX_ABI "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(PQXX_MAJOR "${PROJECT_VERSION_MAJOR}")
set(PQXX_MINOR "${PROJECT_VERSION_MINOR}")

add_custom_target(build-docs ALL
    COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/../src/*.[ch]xx ../src/
    COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/../include/pqxx/* ../include/pqxx/
    COMMAND doxygen ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
    COMMENT Building documentation
)

install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen-html/
    DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
)
