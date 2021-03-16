add_library(
	data-structure STATIC
	"include/queue.h"
	"include/stack.h"
	"src/queue.c"
	"src/stack.c"
)

target_include_directories(data-structure PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(data-structure PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(data-structure PRIVATE DATA-STRUCTURE_VERSION=1)

target_link_libraries(data-structure PRIVATE try)
