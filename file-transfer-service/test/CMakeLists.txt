add_library(test_ftcp OBJECT "ftcp.c")
target_link_libraries(test_ftcp PRIVATE ftcp)
target_include_directories(test_ftcp PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../src")

add_executable(google_test_ftcp "ftcp.cpp")
target_link_libraries(google_test_ftcp PRIVATE GTest::gtest GTest::gtest_main)
target_link_libraries(google_test_ftcp PRIVATE test_ftcp)
gtest_add_tests(TARGET google_test_ftcp)

#add_executable(test_ft_service ft_service.cpp")
#target_link_libraries(test_ft_service PRIVATE GTest::GTest GTest::Main)
#target_link_libraries(test_ft_service PRIVATE ft-service)
#target_include_directories(test_ft_service PRIVATE "../src/")
#gtest_add_tests(TARGET test_ft_service)
