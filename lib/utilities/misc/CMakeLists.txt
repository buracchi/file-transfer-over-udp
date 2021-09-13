add_library(
        utilities-misc
        STATIC
        "src/utilities.c"
)
target_include_directories(utilities-misc PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(utilities-misc INTERFACE coverage_config)
