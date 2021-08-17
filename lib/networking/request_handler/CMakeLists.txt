add_library(
    networking-request-handler
    INTERFACE
)
target_compile_definitions(networking-request-handler INTERFACE REQUEST_HANDLER_VERSION=1)
target_include_directories(networking-request-handler INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(networking-request-handler INTERFACE socket2)
