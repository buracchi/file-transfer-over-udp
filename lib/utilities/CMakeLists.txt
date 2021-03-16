add_library(
	utilities
	STATIC
	"include/utilities.h"
	"src/utilities.c"
)

target_include_directories(utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(utilities PRIVATE UTILITIES_VERSION=1)
