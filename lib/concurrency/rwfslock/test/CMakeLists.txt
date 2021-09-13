add_executable(
        test_rwfslock
        "rwfslock.cpp"
)
target_link_libraries(test_rwfslock PRIVATE GTest::GTest GTest::Main)
target_link_libraries(test_rwfslock PRIVATE concurrency-rwfslock)
gtest_add_tests(TARGET test_rwfslock)
