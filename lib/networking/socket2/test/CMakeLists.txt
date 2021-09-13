add_executable(
        test_socket2
        "socket2.cpp"
)
target_link_libraries(test_socket2 PRIVATE GTest::GTest GTest::Main)
target_link_libraries(test_socket2 PRIVATE networking-socket2)
target_link_libraries(test_socket2 PRIVATE nproto-ipv4)
target_link_libraries(test_socket2 PRIVATE tproto-tcp)
gtest_add_tests(TARGET test_socket2)
