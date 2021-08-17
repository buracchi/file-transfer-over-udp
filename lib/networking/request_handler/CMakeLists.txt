add_library(
    networking-request-handler-lib
    STATIC
    "src/request_handler.c"
)
target_compile_definitions(networking-request-handler-lib PRIVATE REQUEST_HANDLER_VERSION=1)
target_include_directories(networking-request-handler-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(networking-request-handler-lib PUBLIC socket2)
target_link_libraries(networking-request-handler-lib PRIVATE utilities)

if (BUILD_TESTING)
	add_subdirectory("test")
endif()
