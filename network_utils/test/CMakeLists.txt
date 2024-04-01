find_package(buracchi-cutest CONFIG REQUIRED)

add_executable(test_inet_utils "test_inet_utils.c")
target_link_libraries(test_inet_utils PRIVATE network_utils)
target_link_libraries(test_inet_utils PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
target_link_libraries(test_inet_utils INTERFACE coverage_config)
cutest_discover_tests(test_inet_utils)
