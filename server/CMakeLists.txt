find_package(PkgConfig)
pkg_check_modules(liburing REQUIRED IMPORTED_TARGET GLOBAL liburing>=2.0)

add_executable(server src/main.c)
target_link_libraries(server INTERFACE $<BUILD_INTERFACE:coverage_config>)
target_link_libraries(server PRIVATE $<TARGET_OBJECTS:server_obj>)
target_link_libraries(server PRIVATE logger)
target_link_libraries(server PRIVATE network_utils)
target_link_libraries(server PRIVATE tftp)
target_link_libraries(server PRIVATE PkgConfig::liburing)

add_library(server_obj OBJECT
            src/tftp_server.c
            src/tftp_handler.c
            src/tftp_server_stats.c
            src/session_stats.c
            src/uring_thread_pool.c
)
target_include_directories(server_obj PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")
target_link_libraries(server_obj INTERFACE $<BUILD_INTERFACE:coverage_config>)
target_link_libraries(server_obj PRIVATE logger)
target_link_libraries(server_obj PRIVATE network_utils)
target_link_libraries(server_obj PRIVATE tftp)
target_link_libraries(server_obj PRIVATE PkgConfig::liburing)

if (BUILD_TESTS)
    add_subdirectory("test")
endif ()
