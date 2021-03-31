add_executable(
	test_socket2
	"socket2.c"
)

target_link_libraries(test_socket2 PRIVATE socket2)
target_link_libraries(test_socket2 PRIVATE try)

add_test(NAME "Socket2 test" COMMAND "test_socket2")
