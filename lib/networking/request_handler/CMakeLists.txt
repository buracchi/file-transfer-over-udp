add_library(networking-handle-request-lib "src/request_handler.c")
target_include_directories(networking-handle-request-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(networking-handle-request-lib PUBLIC socket2)
