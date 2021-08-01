project(
	"File transfer over UDP Server"
	VERSION 1.0.0
	LANGUAGES C
)

# Executable

add_executable (
	ft-server
	"src/main.c"
)
target_link_libraries(ft-server PRIVATE ft-server-lib)
target_link_libraries(ft-server PRIVATE ft-service-lib)
target_link_libraries(ft-server PRIVATE try)
target_link_libraries(ft-server PRIVATE utilities)

# Libraries

add_library(
	ft-server-lib
	STATIC
	"src/fts.c"
	"src/fts.h"
)
find_package(Libevent CONFIG REQUIRED)
target_link_libraries(ft-server-lib PRIVATE ft-service-lib)
target_link_libraries(ft-server-lib PRIVATE ftcp)
target_link_libraries(ft-server-lib PRIVATE libevent::core libevent::extra libevent::pthreads)
target_link_libraries(ft-server-lib PRIVATE pthread)
target_link_libraries(ft-server-lib PRIVATE socket2)
target_link_libraries(ft-server-lib PRIVATE rwfslock)
target_link_libraries(ft-server-lib PRIVATE tpool)
target_link_libraries(ft-server-lib PRIVATE try)
target_link_libraries(ft-server-lib PRIVATE utilities)
target_include_directories(ft-server-lib PRIVATE "${LIBEVENT_INCLUDE_DIR}")

add_library(
	ft-service-lib
	STATIC
	"src/ft_service.h"
	"src/ft_service.c"
)
target_link_libraries(ft-service-lib PRIVATE try)
target_link_libraries(ft-service-lib PRIVATE utilities)
target_link_libraries(ft-service-lib INTERFACE coverage_config)

# Tests

add_executable (
	ft-service-test
	"test/ft_service.cpp"
)
target_link_libraries(ft-service-test PRIVATE GTest::GTest GTest::Main)
target_link_libraries(ft-service-test PRIVATE ft-service-lib)
target_include_directories(ft-service-test PRIVATE "src/")
gtest_add_tests(TARGET ft-service-test)
