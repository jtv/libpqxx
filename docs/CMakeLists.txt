find_program(HAVE_DOXYGEN doxygen)

if(NOT HAVE_DOXYGEN)
    message(FATAL_ERROR "*****************************************************
Doxygen not found.
Install it, or configure with -DBUILD_DOC=OFF
*****************************************************"
    )
endif()

set(PQXXVERSION "${CMAKE_PROJECT_VERSION}")
set(top_srcdir "${PROJECT_SOURCE_DIR}")
set(PQXX_ABI "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
set(PQXX_MAJOR "${PROJECT_VERSION_MAJOR}")
set(PQXX_MINOR "${PROJECT_VERSION_MINOR}")

find_program(HAVE_DOT dot)
if(HAVE_DOT)
    set(HAVE_DOT YES)
else()
    set(HAVE_DOT NO)
endif()

if(HAVE_DOXYGEN)
    file(
        GLOB DOXYGEN_SOURCES
        "${PROJECT_SOURCE_DIR}/include/pqxx/*.hxx"
        "${PROJECT_SOURCE_DIR}/include/pqxx/doc/*.md"
        "${PROJECT_SOURCE_DIR}/*.cxx"
    )
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doxygen-html
        COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/doxygen-html
        COMMAND doxygen Doxyfile
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile ${DOXYGEN_SOURCES}
        COMMENT "Generate API documentation"
        VERBATIM
    )
    add_custom_target(doxygen ALL
        SOURCES ${DOXYGEN_SOURCES}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/html
        DESTINATION ${CMAKE_INSTALL_DOCDIR}/html
    )
endif()
