add_library(
	socket2
	STATIC
	"include/struct_socket2.h"
	"include/socket2.h"
	"src/socket2.c"
	"include/sockaddr2.h"
	"include/tproto.h"
	"include/nproto.h"
	"include/nproto/nproto_ipv4.h"
	"src/nproto.h/nproto_ipv4.c"
	"include/nproto/nproto_unix.h"
	"src/nproto.h/nproto_unix.c"
 "include/tproto/tproto_tcp.h" "src/tproto.h/tproto_tcp.c" "src/tproto.h/tproto_udp.c")

target_include_directories(socket2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(socket2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(socket2 PRIVATE SOCKET2_VERSION=1)

target_link_libraries(socket2 PRIVATE try)
target_link_libraries(socket2 PRIVATE utilities)
