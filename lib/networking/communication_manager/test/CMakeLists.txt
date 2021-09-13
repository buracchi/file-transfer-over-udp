add_executable(
        test_communication_manager
        "communication_manager.cpp"
)
target_link_libraries(test_communication_manager PRIVATE GTest::GTest GTest::Main)
target_link_libraries(test_communication_manager PRIVATE networking-communication-manager)
target_link_libraries(test_communication_manager PRIVATE nproto-ipv4)
target_link_libraries(test_communication_manager PRIVATE tproto-tcp)
target_link_libraries(test_communication_manager PRIVATE utilities)
gtest_add_tests(TARGET test_communication_manager)
