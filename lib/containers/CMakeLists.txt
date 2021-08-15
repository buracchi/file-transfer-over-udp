add_library(
	containers
	"src/queue/double_stack_queue.c"
)
target_compile_definitions(containers PRIVATE CONTAINERS_VERSION=1)
target_include_directories(containers PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers PUBLIC containers-linked-list-stack)
target_link_libraries(containers PRIVATE utilities)

add_library(
	containers-linked-list-stack
	"src/stack/linked_list_stack.c"
)
target_compile_definitions(containers-linked-list-stack PRIVATE CONTAINERS_VERSION=1)
target_include_directories(containers-linked-list-stack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-linked-list-stack PUBLIC containers-linked-list)
target_link_libraries(containers-linked-list-stack PRIVATE utilities)

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

if (BUILD_TESTING)
	add_subdirectory("test")
endif()
