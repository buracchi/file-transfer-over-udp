#add_subdirectory(request_handler)
#add_subdirectory(communication_manager)
add_subdirectory(socket2)

add_library(networks INTERFACE)
target_compile_definitions(networks INTERFACE NETWORKS_VERSION=1)
target_link_libraries(networks INTERFACE socket2)
