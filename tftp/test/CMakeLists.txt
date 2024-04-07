find_package(buracchi-cutest CONFIG REQUIRED)

add_executable(test_tftp "test.c")
target_link_libraries(test_tftp PRIVATE tftp)
target_link_libraries(test_tftp PRIVATE buracchi::cutest::cutest buracchi::cutest::cutest_main)
target_link_libraries(test_tftp INTERFACE coverage_config)
cutest_discover_tests(test_tftp)
