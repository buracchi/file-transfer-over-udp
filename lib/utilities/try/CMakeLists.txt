add_library(utilities-try INTERFACE)
target_include_directories(utilities-try INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(utilities-try INTERFACE coverage_config)
