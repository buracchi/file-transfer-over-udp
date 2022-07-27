add_library(
	test_ftcp
	OBJECT
	"ftcp.c"
)
target_link_libraries(test_ftcp PRIVATE ftcp)

add_executable(
		google_test_ftcp
		"ftcp.cpp"
)
target_link_libraries(google_test_ftcp PRIVATE GTest::gtest GTest::gtest_main)
target_link_libraries(google_test_ftcp PRIVATE test_ftcp)
gtest_add_tests(TARGET google_test_ftcp)
