add_library(
        concurrency-rwfslock
        STATIC
        "src/rwfslock.c"
)
target_include_directories(concurrency-rwfslock PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(concurrency-rwfslock PRIVATE containers-linked-list-map)
target_link_libraries(concurrency-rwfslock PRIVATE pthread)
target_link_libraries(concurrency-rwfslock PRIVATE utilities)
target_link_libraries(concurrency-rwfslock INTERFACE coverage_config)

if (BUILD_TESTING)
    add_subdirectory("test")
endif ()
