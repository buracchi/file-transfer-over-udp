add_library(
	rwfslock
	STATIC
	"include/rwfslock.h"
	"src/rwfslock.c"
)

target_include_directories(rwfslock PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(rwfslock PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

target_compile_definitions(rwfslock PRIVATE RWFSLOCK_VERSION=1)

target_link_libraries(rwfslock PRIVATE data-structure)
target_link_libraries(rwfslock PRIVATE pthread)
target_link_libraries(rwfslock PRIVATE try)
target_link_libraries(rwfslock PRIVATE utilities)
