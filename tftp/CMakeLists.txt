find_package(PkgConfig)
pkg_check_modules(liburing REQUIRED IMPORTED_TARGET GLOBAL liburing>=2.0)

add_library(tftp STATIC
    src/tftp.c
    src/client/tftp_client.c
    src/client/tftp_client_connection.c
    src/client/tftp_client_stats.c
    src/server/tftp_server.c
    src/server/tftp_server_listener.c
    src/server/tftp_handler.c
    src/server/tftp_server_stats.c
    src/server/tftp_server_thread_pool.c
    src/server/tftp_session.c
    src/server/tftp_session_stats.c
    src/server/dispatcher.c
    src/utils/inet.c
)
target_include_directories(tftp
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE ${liburing_INCLUDE_DIRS})
target_link_libraries(tftp
    INTERFACE $<BUILD_INTERFACE:coverage_config>
    PUBLIC logger
    PUBLIC PkgConfig::liburing
)

if (BUILD_TESTS)
    add_subdirectory("test")
endif ()
