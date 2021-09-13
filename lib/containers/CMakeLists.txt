add_library(containers INTERFACE)
target_compile_definitions(containers INTERFACE CONTAINERS_VERSION=1)
target_link_libraries(containers INTERFACE containers-list)
target_link_libraries(containers INTERFACE containers-stack)
target_link_libraries(containers INTERFACE containers-queue)
target_link_libraries(containers INTERFACE containers-map)
target_link_libraries(containers INTERFACE containers-linked-list)
target_link_libraries(containers INTERFACE containers-linked-list-stack)
target_link_libraries(containers INTERFACE containers-double-linked-list-stack-queue)
target_link_libraries(containers INTERFACE containers-linked-list-map)

add_library(
        containers-list
        "src/list.c"
)
target_include_directories(containers-list PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-list PUBLIC iterators)
target_link_libraries(containers-list INTERFACE coverage_config)

add_library(
        containers-stack
        "src/stack.c"
)
target_include_directories(containers-stack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-stack INTERFACE coverage_config)

add_library(
        containers-queue
        "src/queue.c"
)
target_include_directories(containers-queue PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-queue INTERFACE coverage_config)

add_library(
        containers-map
        "src/map.c"
)
target_include_directories(containers-map PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-map PUBLIC iterators)
target_link_libraries(containers-map INTERFACE coverage_config)

add_library(
        containers-linked-list
        "src/list/linked_list.c"
        "src/list/linked_list_element.c"
        "src/list/linked_list_iterator.c"
)
target_include_directories(containers-linked-list PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-linked-list PUBLIC containers-list)
target_link_libraries(containers-linked-list PRIVATE utilities-try)
target_link_libraries(containers-linked-list INTERFACE coverage_config)

add_library(
        containers-linked-list-stack
        "src/stack/linked_list_stack.c"
)
target_include_directories(containers-linked-list-stack PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-linked-list-stack PUBLIC containers-stack)
target_link_libraries(containers-linked-list-stack PRIVATE containers-linked-list)
target_link_libraries(containers-linked-list-stack PRIVATE utilities-try)
target_link_libraries(containers-linked-list-stack INTERFACE coverage_config)

add_library(
        containers-double-linked-list-stack-queue
        "src/queue/double_linked_list_stack_queue.c"
)
target_include_directories(containers-double-linked-list-stack-queue PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-double-linked-list-stack-queue PUBLIC containers-queue)
target_link_libraries(containers-double-linked-list-stack-queue PRIVATE containers-linked-list-stack)
target_link_libraries(containers-double-linked-list-stack-queue PRIVATE utilities-try)
target_link_libraries(containers-double-linked-list-stack-queue INTERFACE coverage_config)

add_library(
        containers-linked-list-map
        "src/map/linked_list_map.c"
)
target_compile_definitions(containers-linked-list-map PRIVATE CONTAINERS_VERSION=1)
target_include_directories(containers-linked-list-map PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(containers-linked-list-map PUBLIC containers-map)
target_link_libraries(containers-linked-list-map PRIVATE containers-linked-list)
target_link_libraries(containers-linked-list-map PRIVATE utilities-misc)
target_link_libraries(containers-linked-list-map PRIVATE utilities-try)
target_link_libraries(containers-linked-list-map INTERFACE coverage_config)

if (BUILD_TESTING)
    add_subdirectory("test")
endif ()
