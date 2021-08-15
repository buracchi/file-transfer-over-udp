add_library(
	rwfslock
	STATIC
	"src/rwfslock.c"
)
target_compile_definitions(rwfslock PRIVATE RWFSLOCK_VERSION=1)
target_include_directories(rwfslock PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rwfslock PUBLIC containers-linked-list-map)
target_link_libraries(rwfslock PRIVATE pthread)
target_link_libraries(rwfslock PRIVATE utilities)

if (BUILD_TESTING)
	add_subdirectory("test")
endif()
