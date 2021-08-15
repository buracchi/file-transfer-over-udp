add_executable (
	double-stack-queue-test
	"queue/double_stack_queue.cpp"
)
target_link_libraries(double-stack-queue-test PRIVATE GTest::GTest GTest::Main)
target_link_libraries(double-stack-queue-test PRIVATE containers)
gtest_add_tests(TARGET double-stack-queue-test)

add_executable (
	linked-list-stack-test
	"stack/linked_list_stack.cpp"
)
target_link_libraries(linked-list-stack-test PRIVATE GTest::GTest GTest::Main)
target_link_libraries(linked-list-stack-test PRIVATE containers-linked-list-stack)
gtest_add_tests(TARGET linked-list-stack-test)

add_executable (
	linked-list-test
	"list/linked_list.cpp"
)
target_link_libraries(linked-list-test PRIVATE GTest::GTest GTest::Main)
target_link_libraries(linked-list-test PRIVATE containers-linked-list)
gtest_add_tests(TARGET linked-list-test)
