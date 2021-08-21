add_library(
	tpool
	STATIC
	"src/tpool.c"
)
target_include_directories(tpool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tpool PRIVATE containers-double-linked-list-stack-queue)
target_link_libraries(tpool PRIVATE pthread)
target_link_libraries(tpool PRIVATE utilities)
target_link_libraries(tpool INTERFACE coverage_config)

if (BUILD_TESTING)
	add_subdirectory("test")
endif()
