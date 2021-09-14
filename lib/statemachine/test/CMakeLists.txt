add_executable(
        test_statemachine
        "statemachine.cpp"
)
target_link_libraries(test_statemachine PRIVATE GTest::GTest GTest::Main)
target_link_libraries(test_statemachine PRIVATE statemachine)
gtest_add_tests(TARGET test_statemachine)
