find_package(PkgConfig)
pkg_check_modules(liburing REQUIRED IMPORTED_TARGET GLOBAL liburing>=2.0)

add_library(tftp STATIC
    src/tftp.c
    src/client/client.c
    src/client/connection.c
    src/client/stats.c
    src/server/adaptive_timeout.c
    src/server/server.c
    src/server/listener.c
    src/server/server_stats.c
    src/server/worker_pool.c
    src/server/session.c
    src/server/session_file.c
    src/server/session_options.c
    src/server/session_stats.c
    src/server/session_connection.c
    src/server/dispatcher.c
    src/server/worker.c
    src/server/worker_job.c
    src/utils/inet.c
    src/utils/io.c
)
target_include_directories(tftp
    PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
           "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    PRIVATE ${liburing_INCLUDE_DIRS})
target_link_libraries(tftp
    INTERFACE $<BUILD_INTERFACE:coverage_config>
    PRIVATE m
    PUBLIC logger
    PUBLIC PkgConfig::liburing
)

if (BUILD_TESTS)
    add_subdirectory("test")
endif ()
