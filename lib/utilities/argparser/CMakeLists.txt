add_library(
        utilities-argparser
        STATIC
        "src/argparser.c"
        "src/parser.c"
        "src/usage.c"
)
target_include_directories(utilities-argparser PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(utilities-argparser PUBLIC containers-map)
target_link_libraries(utilities-argparser PRIVATE containers-linked-list)
target_link_libraries(utilities-argparser PRIVATE containers-linked-list-map)
target_link_libraries(utilities-argparser PRIVATE utilities-misc)
target_link_libraries(utilities-argparser PRIVATE utilities-try)
target_link_libraries(utilities-argparser INTERFACE coverage_config)

if (BUILD_TESTING)
    add_subdirectory("test")
endif ()
