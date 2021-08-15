add_executable (
	rwfslock-test
	"rwfslock.cpp"
)
target_link_libraries(rwfslock-test PRIVATE GTest::GTest GTest::Main)
target_link_libraries(rwfslock-test PRIVATE rwfslock)
gtest_add_tests(TARGET rwfslock-test)
