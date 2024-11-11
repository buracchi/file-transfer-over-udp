find_package(buracchi-cutest CONFIG REQUIRED)

add_executable(tftp_test "test.c")
target_link_libraries(tftp_test
    PRIVATE tftp
    PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
cutest_discover_tests(tftp_test)

add_executable(tftp_test_utils_inet "test_utils_inet.c")
target_link_libraries(tftp_test_utils_inet
    PRIVATE tftp
    PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
cutest_discover_tests(tftp_test_utils_inet)

add_executable(tftp_test_server "test_server.c")
target_link_libraries(tftp_test_server
    PRIVATE tftp
    PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
cutest_discover_tests(tftp_test_server)

add_executable(tftp_test_client "test_client.c")
target_link_libraries(tftp_test_client
    PRIVATE tftp
    PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
cutest_discover_tests(tftp_test_client)
target_link_options(tftp_test_client PRIVATE
    -Wl,--wrap=logger_init
    -Wl,--wrap=logger_destroy
    -Wl,--wrap=logger_log)

add_executable(tftp_test_server_listener "test_server_listener.c")
target_link_libraries(tftp_test_server_listener
    PRIVATE tftp
    PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
target_link_options(tftp_test_server_listener PRIVATE
    -Wl,--wrap=logger_init
    -Wl,--wrap=logger_destroy
    -Wl,--wrap=logger_log)
cutest_discover_tests(tftp_test_server_listener)

add_executable(tftp_test_server_dispatcher "test_server_dispatcher.c")
target_link_libraries(tftp_test_server_dispatcher
    PRIVATE tftp
    PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
cutest_discover_tests(tftp_test_server_dispatcher)
