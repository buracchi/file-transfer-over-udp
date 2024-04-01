find_package(buracchi-argparser CONFIG REQUIRED)
find_package(buracchi-common CONFIG REQUIRED)
find_package(libevent CONFIG REQUIRED)

add_executable(client
        "src/main.c"
        "src/cmdline.c"
        "src/connector.c"
        "src/service_handler.c")
target_link_libraries(client PRIVATE buracchi::argparser::argparser)
target_link_libraries(client PRIVATE buracchi::common::logger)
target_link_libraries(client PRIVATE buracchi::common::utilities)
target_link_libraries(client PRIVATE libevent::core)
target_link_libraries(client PRIVATE network_utils)
target_link_libraries(client INTERFACE $<BUILD_INTERFACE:coverage_config>)
