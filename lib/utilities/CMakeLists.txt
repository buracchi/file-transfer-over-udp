add_library(
	utilities
	STATIC
	"src/utilities.c"
)
target_compile_definitions(utilities PRIVATE UTILITIES_VERSION=1)
target_include_directories(utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
