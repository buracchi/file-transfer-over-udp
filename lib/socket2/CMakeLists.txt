add_library(
	socket2
	STATIC
	"include/socket2.h"
	"src/socket2.c"
)

target_include_directories(socket2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(socket2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(socket2 PRIVATE SOCKET2_VERSION=1)

target_link_libraries(socket2 PRIVATE try)
target_link_libraries(socket2 PRIVATE utilities)
