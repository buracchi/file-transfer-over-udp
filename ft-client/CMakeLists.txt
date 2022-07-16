find_package(buracchi-common CONFIG REQUIRED)

add_executable(
        ft-client
        "src/main.c"
)
target_link_libraries(ft-client PRIVATE ftcp)
target_link_libraries(ft-client PRIVATE buracchi::common::argparser)
target_link_libraries(ft-client PRIVATE buracchi::common::networking)
target_link_libraries(ft-client PRIVATE buracchi::common::utilities)
#target_link_libraries(ft-client PRIVATE tproto-gbn)
target_link_libraries(ft-client INTERFACE coverage_config)
