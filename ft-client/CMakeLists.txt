project(
	"File transfer over UDP Client"
	VERSION 1.0.0
	LANGUAGES C
)

add_executable (
	ft-client
	"src/main.c"
)
target_link_libraries(ft-client PRIVATE ft-client-lib)
target_link_libraries(ft-client PRIVATE utilities)
target_link_libraries(ft-client INTERFACE coverage_config)

add_library(
	ft-client-lib
	STATIC
	"src/ftc.h"
	"src/ftc.c"
)
target_link_libraries(ft-client-lib PRIVATE ftcp)
target_link_libraries(ft-client-lib PRIVATE networking-socket2)
target_link_libraries(ft-client-lib PRIVATE nproto-ipv4)
target_link_libraries(ft-client-lib PRIVATE tproto-tcp) #remove this
target_link_libraries(ft-client-lib PRIVATE tproto-gbn)
target_link_libraries(ft-client-lib PRIVATE utilities)
target_link_libraries(ft-client-lib INTERFACE coverage_config)
