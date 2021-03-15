add_library(
	data-structure STATIC
	
	"include/data-structure/queue.h"
	"include/data-structure/stack.h"
	
	"src/data-structure/queue.c"
	"src/data-structure/stack.c"
)

target_include_directories(data-structure PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/data-structure")
target_include_directories(data-structure PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src/data-structure")

target_compile_definitions(data-structure PRIVATE DATA-STRUCTURE_VERSION=1)

target_link_libraries(data-structure PRIVATE try)

add_library(
	socket2
	STATIC
	"include/socket2.h"
	"src/socket2/socket2.c"
)

target_include_directories(socket2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(socket2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(socket2 PRIVATE SOCKET2_VERSION=1)

target_link_libraries(socket2 PRIVATE try)

add_library(
	tpool
	STATIC
	"include/tpool.h"
	"src/tpool/tpool.c"
)

target_include_directories(tpool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(tpool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(tpool PRIVATE TPOOL_VERSION=1)

target_link_libraries(tpool PRIVATE data-structure)
target_link_libraries(tpool PUBLIC pthread)
target_link_libraries(tpool PRIVATE try)

add_library(
	try
	INTERFACE
)

target_include_directories(try INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

add_library(
	utilities
	STATIC
	"include/utilities.h"
	"src/utilities/utilities.c"
)

target_include_directories(utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(utilities PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(utilities PRIVATE RESOURCES_VERSION=1)
