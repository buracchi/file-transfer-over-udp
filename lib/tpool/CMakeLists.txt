add_library(
	tpool
	STATIC
	"include/tpool.h"
	"src/tpool.c"
)

target_include_directories(tpool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(tpool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(tpool PRIVATE TPOOL_VERSION=1)

target_link_libraries(tpool PRIVATE data-structure)
target_link_libraries(tpool PRIVATE pthread)
target_link_libraries(tpool PRIVATE try)
target_link_libraries(tpool PRIVATE utilities)
