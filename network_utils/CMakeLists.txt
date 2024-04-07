add_library(network_utils STATIC "src/inet_utils.c")
target_include_directories(network_utils PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(network_utils INTERFACE $<BUILD_INTERFACE:coverage_config>)

if (BUILD_TESTS)
    add_subdirectory("test")
endif ()
