cmake_minimum_required (VERSION 3.8)

project(
	"File transfer over UDP Server"
	VERSION 1.0.0
	LANGUAGES C
)

# Executable

add_executable (
	ft-server
	"src/main/c/main.c"
)
target_link_libraries(ft-server PRIVATE ft-server-lib)
target_link_libraries(ft-server PRIVATE try)
target_link_libraries(ft-server PRIVATE utilities)
add_dependencies(ft-server ft-server-lib)

# Libraries

add_library(
	ft-server-lib
	STATIC
	"src/main/c/fts.c"
	"src/main/c/fts.h"
)
find_package(Libevent CONFIG REQUIRED)
target_link_libraries(ft-server-lib PRIVATE ftcp)
target_link_libraries(ft-server-lib PRIVATE libevent::core libevent::extra libevent::pthreads)
target_link_libraries(ft-server-lib PRIVATE pthread)
target_link_libraries(ft-server-lib PRIVATE socket2)
target_link_libraries(ft-server-lib PRIVATE rwfslock)
target_link_libraries(ft-server-lib PRIVATE tpool)
target_link_libraries(ft-server-lib PRIVATE try)
target_link_libraries(ft-server-lib PRIVATE utilities)
target_include_directories(ft-server-lib PRIVATE "${LIBEVENT_INCLUDE_DIR}")

# Tests

add_executable (
	ft-server-test
	"src/test/c/fts_test.c"
)
target_link_libraries(ft-server-test PRIVATE ft-server-lib)
target_include_directories(ft-server-test PRIVATE "src/main/c")

add_test(
	NAME SERVER_TEST 
	COMMAND ft-server-test
	#[<args>...]
)
