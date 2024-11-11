find_package(buracchi-cutest CONFIG REQUIRED)

add_executable(logger_test "test_logger.c")
target_link_libraries(logger_test
    PRIVATE logger
    PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main
    INTERFACE coverage_config)
cutest_discover_tests(logger_test)
