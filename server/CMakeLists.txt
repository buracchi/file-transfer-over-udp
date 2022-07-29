find_package(buracchi-common CONFIG REQUIRED)

add_executable(ft-server "src/main.c")
target_link_libraries(ft-server PRIVATE file-transfer-service)
target_link_libraries(ft-server PRIVATE buracchi::common::argparser)
target_link_libraries(ft-server PRIVATE buracchi::common::networking)
target_link_libraries(ft-server PRIVATE buracchi::common::utilities)
#target_link_libraries(server PRIVATE tproto-gbn)
target_link_libraries(ft-server INTERFACE coverage_config)
