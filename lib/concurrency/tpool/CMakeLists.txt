add_library(
        concurrency-tpool
        STATIC
        "src/tpool.c"
)
target_include_directories(concurrency-tpool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(concurrency-tpool PRIVATE containers-double-linked-list-stack-queue)
target_link_libraries(concurrency-tpool PRIVATE pthread)
target_link_libraries(concurrency-tpool PRIVATE utilities)
target_link_libraries(concurrency-tpool INTERFACE coverage_config)

if (BUILD_TESTING)
    add_subdirectory("test")
endif ()
