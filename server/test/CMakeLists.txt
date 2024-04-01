find_package(buracchi-cutest CONFIG REQUIRED)

add_executable(test_acceptor "test_acceptor.c")
target_link_libraries(test_acceptor PRIVATE server_obj)
target_link_libraries(test_acceptor PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
target_link_libraries(test_acceptor INTERFACE coverage_config)
cutest_discover_tests(test_acceptor)
