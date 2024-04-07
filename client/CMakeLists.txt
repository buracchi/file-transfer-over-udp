add_executable(client
               src/main.c
               src/tftp_client.c
               src/tftp_client_stats.c)
target_link_libraries(client INTERFACE $<BUILD_INTERFACE:coverage_config>)
target_link_libraries(client PRIVATE network_utils)
target_link_libraries(client PRIVATE logger)
target_link_libraries(client PRIVATE network_utils)
target_link_libraries(client PRIVATE tftp)
