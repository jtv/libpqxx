file(GLOB CXX_SOURCES "*.cxx")

if(NOT SKIP_PQXX_SHARED)
	# Build a shared library
	add_library(pqxx_shared SHARED ${CXX_SOURCES})
	target_compile_definitions(pqxx_shared PUBLIC -DPQXX_SHARED)
	set_target_properties(pqxx_shared PROPERTIES OUTPUT_NAME pqxx)
endif()

if(NOT SKIP_PQXX_STATIC)
	# Build a static libary
	add_library(pqxx_static STATIC ${CXX_SOURCES})
	set_target_properties(pqxx_static PROPERTIES OUTPUT_NAME pqxx)
endif()


if(WIN32)
    target_link_libraries(pqxx_static PUBLIC wsock32 ws2_32)
endif()
