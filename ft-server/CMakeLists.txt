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
target_link_libraries(ft-server PRIVATE ft-service-lib)
target_link_libraries(ft-server PRIVATE ft-handler-lib)
target_link_libraries(ft-server PRIVATE connection-listener-lib)
target_link_libraries(ft-server PRIVATE socket2)
target_link_libraries(ft-server PRIVATE try)
target_link_libraries(ft-server PRIVATE utilities)

# Libraries

add_library(
	ft-service-lib
	STATIC
	"src/ft_service.h"
	"src/ft_service.c"
)
target_link_libraries(ft-service-lib PRIVATE try)
target_link_libraries(ft-service-lib PRIVATE utilities)
target_link_libraries(ft-service-lib INTERFACE coverage_config)

add_library(
	ft-handler-lib
	STATIC
	"src/ft_handler.c"
	"src/ft_handler.h"
)
target_link_libraries(ft-handler-lib PRIVATE ft-service-lib)
target_link_libraries(ft-handler-lib PRIVATE ftcp)
target_link_libraries(ft-handler-lib PRIVATE socket2)
target_link_libraries(ft-handler-lib PRIVATE rwfslock)
target_link_libraries(ft-handler-lib PRIVATE try)
target_link_libraries(ft-handler-lib PRIVATE utilities)

add_library(
	connection-listener-lib
	STATIC
	"src/connection_listener.c"
	"src/connection_listener.h"
)
find_package(Libevent CONFIG REQUIRED)
target_link_libraries(connection-listener-lib PRIVATE ft-handler-lib)
target_link_libraries(connection-listener-lib PRIVATE libevent::core libevent::extra libevent::pthreads)
target_link_libraries(connection-listener-lib PRIVATE pthread)
target_link_libraries(connection-listener-lib PRIVATE socket2)
target_link_libraries(connection-listener-lib PRIVATE tpool)
target_link_libraries(connection-listener-lib PRIVATE try)
target_link_libraries(connection-listener-lib PRIVATE utilities)
target_include_directories(connection-listener-lib PRIVATE "${LIBEVENT_INCLUDE_DIR}")

# Tests

add_executable (
	ft-service-test
	"test/ft_service.cpp"
)
target_link_libraries(ft-service-test PRIVATE GTest::GTest GTest::Main)
target_link_libraries(ft-service-test PRIVATE ft-service-lib)
target_include_directories(ft-service-test PRIVATE "src/")
gtest_add_tests(TARGET ft-service-test)
