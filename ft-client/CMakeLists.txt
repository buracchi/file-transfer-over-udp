project(
	"File transfer over UDP Client"
	VERSION 1.0.0
	LANGUAGES C
)

# Executable

add_executable (
	ft-client
	"src/main/c/main.c"
)
target_link_libraries(ft-client PRIVATE ft-client-lib)
target_link_libraries(ft-client PRIVATE try)
target_link_libraries(ft-client PRIVATE utilities)
add_dependencies(ft-client ft-client-lib)

# Libraries

add_library(
	ft-client-lib
	STATIC
	"src/main/c/ftc.h"
	"src/main/c/ftc.c"
)
target_link_libraries(ft-client-lib PRIVATE ftcp)
target_link_libraries(ft-client-lib PRIVATE socket2)
target_link_libraries(ft-client-lib PRIVATE try)
target_link_libraries(ft-client-lib PRIVATE utilities)

# Tests

add_executable (
	ft-client-test
	"src/test/c/ftc_test.c"
)
target_link_libraries(ft-client-test PRIVATE ft-client-lib)
target_include_directories(ft-client-test PRIVATE "src/main/c")

add_test(
	NAME FIRST_TEST 
	COMMAND ft-client-test
	#[<args>...]
)
