add_library(
	containers-linked-list-map
	"src/map/linked_list_map.c"
)
target_compile_definitions(containers-linked-list-map PRIVATE CONTAINERS_VERSION=1)
target_include_directories(containers-linked-list-map PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-linked-list-map PUBLIC containers-linked-list)
target_link_libraries(containers-linked-list-map PRIVATE utilities)
target_link_libraries(containers-linked-list-map INTERFACE coverage_config)

add_library(
	containers-double-stack-queue
	"src/queue/double_stack_queue.c"
)
target_compile_definitions(containers-double-stack-queue PRIVATE CONTAINERS_VERSION=1)
target_include_directories(containers-double-stack-queue PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-double-stack-queue PUBLIC containers-linked-list-stack)
target_link_libraries(containers-double-stack-queue PRIVATE utilities)
target_link_libraries(containers-double-stack-queue INTERFACE coverage_config)

add_library(
	containers-linked-list-stack
	"src/stack/linked_list_stack.c"
)
target_compile_definitions(containers-linked-list-stack PRIVATE CONTAINERS_VERSION=1)
target_include_directories(containers-linked-list-stack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-linked-list-stack PUBLIC containers-linked-list)
target_link_libraries(containers-linked-list-stack PRIVATE utilities)
target_link_libraries(containers-linked-list-stack INTERFACE coverage_config)

add_library(
	containers-linked-list
	"src/list/linked_list.c"
	"src/list/linked_list_element.c"
	"src/list/linked_list_iterator.c"
)
target_compile_definitions(containers-linked-list PRIVATE CONTAINERS_VERSION=1)
target_include_directories(containers-linked-list PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-linked-list PUBLIC iterators)
target_link_libraries(containers-linked-list PRIVATE utilities)
target_link_libraries(containers-linked-list INTERFACE coverage_config)

if (BUILD_TESTING)
	add_subdirectory("test")
endif()
