add_library(tftp STATIC "src/tftp.c")
target_include_directories(tftp PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(tftp INTERFACE $<BUILD_INTERFACE:coverage_config>)
target_link_libraries(tftp PRIVATE network_utils)

if (BUILD_TESTS)
    add_subdirectory("test")
endif ()
