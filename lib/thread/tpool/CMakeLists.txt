add_library(
	tpool
	STATIC
	"src/tpool.c"
)
target_compile_definitions(tpool PRIVATE TPOOL_VERSION=1)
target_include_directories(tpool PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tpool PUBLIC containers-double-stack-queue)
target_link_libraries(tpool PRIVATE pthread)
target_link_libraries(tpool PRIVATE utilities)
target_link_libraries(tpool INTERFACE coverage_config)

if (BUILD_TESTING)
	add_subdirectory("test")
endif()
