add_executable (
	test_tproto_gbn
	"tproto_gbn.cpp"
)
target_link_libraries(test_tproto_gbn PRIVATE GTest::GTest GTest::Main)
target_link_libraries(test_tproto_gbn PRIVATE networking-socket2)
target_link_libraries(test_tproto_gbn PRIVATE nproto-ipv4)
target_link_libraries(test_tproto_gbn PRIVATE tproto-gbn)
gtest_add_tests(TARGET test_tproto_gbn)
