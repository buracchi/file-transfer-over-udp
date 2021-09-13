add_executable(
        test_tpool
        "tpool.cpp"
)
target_link_libraries(test_tpool PRIVATE GTest::GTest GTest::Main)
target_link_libraries(test_tpool PRIVATE concurrency-tpool)
gtest_add_tests(TARGET test_tpool)
