add_executable(benchmark benchmark.c)
target_link_libraries(benchmark
                      PRIVATE logger
                      PRIVATE tftp)

add_dependencies(benchmark server)

add_custom_command(
    TARGET benchmark POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
            $<TARGET_FILE:server>
            $<TARGET_FILE_DIR:benchmark>
    COMMENT "Copying server executable to benchmark directory"
)

# Create test files for benchmarking
add_custom_command(
    TARGET benchmark POST_BUILD
    COMMAND dd if=/dev/zero of=${CMAKE_CURRENT_BINARY_DIR}/1MB bs=1M count=1
    COMMAND dd if=/dev/zero of=${CMAKE_CURRENT_BINARY_DIR}/10MB bs=1M count=10
    COMMAND dd if=/dev/zero of=${CMAKE_CURRENT_BINARY_DIR}/100MB bs=1M count=100
    COMMENT "Creating test files for benchmarking"
)
