project(
	"Go Back N Deamon"
	VERSION 1.0.0
	LANGUAGES C
)

# Executable

add_executable (
	gbnd
	"src/main.c"
)
target_link_libraries(gbnd PRIVATE gbnd-lib)
target_link_libraries(gbnd PRIVATE try)
target_link_libraries(gbnd PRIVATE utilities)
add_dependencies(gbnd gbnd-lib)

# Libraries

add_library(
	gbnd-lib
	STATIC
	"src/gbnd.c"
	"src/gbnd.h"
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
