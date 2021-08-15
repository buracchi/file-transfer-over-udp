add_executable (
	tpool-test
	"tpool.cpp"
)
target_link_libraries(tpool-test PRIVATE GTest::GTest GTest::Main)
target_link_libraries(tpool-test PRIVATE tpool)
gtest_add_tests(TARGET tpool-test)
