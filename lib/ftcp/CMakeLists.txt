add_library(
	ftcp
	STATIC
	"include/ftcp.h"
	"src/ftcp.c"
)

target_include_directories(ftcp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(ftcp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(ftcp PRIVATE FTCP_VERSION=1)

target_link_libraries(ftcp PRIVATE try)
