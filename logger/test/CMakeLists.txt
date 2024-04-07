find_package(buracchi-cutest CONFIG REQUIRED)

add_executable(test_logger "test_logger.c")
target_link_libraries(test_logger PRIVATE logger)
target_link_libraries(test_logger PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
target_link_libraries(test_logger INTERFACE coverage_config)
cutest_discover_tests(test_logger)
