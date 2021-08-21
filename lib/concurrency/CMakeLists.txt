add_subdirectory("rwfslock")
add_subdirectory("tpool")

add_library(concurrency INTERFACE)
target_compile_definitions(concurrency INTERFACE CONCURRENCY_VERSION=1)
target_link_libraries(concurrency INTERFACE concurrency-rwfslock)
target_link_libraries(concurrency INTERFACE concurrency-tpool)
