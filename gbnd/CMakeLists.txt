project(
	"Go Back N Deamon"
	VERSION 1.0.0
	LANGUAGES C
)

# Executable

add_executable (
	gbnd
	"src/main/c/main.c"
)
target_link_libraries(gbnd PRIVATE gbnd-lib)
target_link_libraries(gbnd PRIVATE try)
target_link_libraries(gbnd PRIVATE utilities)
add_dependencies(gbnd gbnd-lib)

# Libraries

add_library(
	gbnd-lib
	STATIC
	"src/main/c/gbnd.c"
	"src/main/c/gbnd.h"
)
find_package(Libevent CONFIG REQUIRED)
target_link_libraries(gbnd-lib PRIVATE ftcp)
target_link_libraries(gbnd-lib PRIVATE libevent::core libevent::extra libevent::pthreads)
target_link_libraries(gbnd-lib PRIVATE pthread)
target_link_libraries(gbnd-lib PRIVATE socket2)
target_link_libraries(gbnd-lib PRIVATE rwfslock)
target_link_libraries(gbnd-lib PRIVATE tpool)
target_link_libraries(gbnd-lib PRIVATE try)
target_link_libraries(gbnd-lib PRIVATE utilities)
target_include_directories(gbnd-lib PRIVATE "${LIBEVENT_INCLUDE_DIR}")

# Tests

add_executable (
	gbnd-test
	"src/test/c/gbnd_test.c"
)
target_link_libraries(gbnd-test PRIVATE gbnd-lib)
target_include_directories(gbnd-test PRIVATE "src/main/c")

add_test(
	NAME GBND_TEST 
	COMMAND gbnd-test
	#[<args>...]
)
