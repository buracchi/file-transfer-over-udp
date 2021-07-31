project(
	"File transfer over UDP Client"
	VERSION 1.0.0
	LANGUAGES C
)

# Executable

add_executable (
	ft-client
	"src/main.c"
)
target_link_libraries(ft-client PRIVATE ft-client-lib)
target_link_libraries(ft-client PRIVATE try)
target_link_libraries(ft-client PRIVATE utilities)
add_dependencies(ft-client ft-client-lib)

# Libraries

add_library(
	ft-client-lib
	STATIC
	"src/ftc.h"
	"src/ftc.c"
)
target_link_libraries(ft-client-lib PRIVATE ftcp)
target_link_libraries(ft-client-lib PRIVATE socket2)
target_link_libraries(ft-client-lib PRIVATE try)
target_link_libraries(ft-client-lib PRIVATE utilities)

# Tests
