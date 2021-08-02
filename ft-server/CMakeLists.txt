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
target_link_libraries(ft-server PRIVATE networking-communication-manager-lib)
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
	"src/struct_ft_handler.h"
)
target_link_libraries(ft-handler-lib PRIVATE networking-handle-request-lib)
target_link_libraries(ft-handler-lib PRIVATE ft-service-lib)
target_link_libraries(ft-handler-lib PRIVATE ftcp)
target_link_libraries(ft-handler-lib PRIVATE socket2)
target_link_libraries(ft-handler-lib PRIVATE rwfslock)
target_link_libraries(ft-handler-lib PRIVATE try)
target_link_libraries(ft-handler-lib PRIVATE utilities)

# Tests

add_executable (
	ft-service-test
	"test/ft_service.cpp"
)
target_link_libraries(ft-service-test PRIVATE GTest::GTest GTest::Main)
target_link_libraries(ft-service-test PRIVATE ft-service-lib)
target_include_directories(ft-service-test PRIVATE "src/")
gtest_add_tests(TARGET ft-service-test)
