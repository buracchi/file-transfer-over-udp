add_executable(
        test_ft_service
        "ft_service.cpp"
)
target_link_libraries(test_ft_service PRIVATE GTest::GTest GTest::Main)
target_link_libraries(test_ft_service PRIVATE ft-service)
target_include_directories(test_ft_service PRIVATE "../src/")
gtest_add_tests(TARGET test_ft_service)
