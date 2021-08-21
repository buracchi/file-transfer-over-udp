add_library(
	rwfslock
	STATIC
	"src/rwfslock.c"
)
target_include_directories(rwfslock PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(rwfslock PRIVATE containers-linked-list-map)
target_link_libraries(rwfslock PRIVATE pthread)
target_link_libraries(rwfslock PRIVATE utilities)
target_link_libraries(rwfslock INTERFACE coverage_config)

if (BUILD_TESTING)
	add_subdirectory("test")
endif()
