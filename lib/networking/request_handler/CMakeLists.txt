add_library(
        networking-request-handler
        STATIC
        "src/request_handler.c"
)
target_include_directories(networking-request-handler PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(networking-request-handler PUBLIC networking-socket2)
