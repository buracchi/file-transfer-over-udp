add_library(
	main-mock
	"main-mock.c"
	"main-mock.h"
)
target_link_libraries(main-mock PRIVATE utilities-argparser)

add_executable (
	test_utilities_argparser
	"argparser.cpp"
)
target_link_libraries(test_utilities_argparser PRIVATE GTest::GTest GTest::Main)
target_link_libraries(test_utilities_argparser PRIVATE main-mock)
gtest_add_tests(TARGET test_utilities_argparser)
