find_package(Libevent CONFIG REQUIRED)

add_library(
    networking-communication-manager 
    STATIC 
    "src/communication_manager.c"
)
target_include_directories(networking-communication-manager PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(networking-communication-manager PRIVATE "${LIBEVENT_INCLUDE_DIR}")
target_link_libraries(networking-communication-manager PUBLIC networking-request-handler)
target_link_libraries(networking-communication-manager PRIVATE networking-socket2)
target_link_libraries(networking-communication-manager PRIVATE nproto-ipv4)
target_link_libraries(networking-communication-manager PRIVATE tproto-tcp)
target_link_libraries(networking-communication-manager PRIVATE libevent::core libevent::extra libevent::pthreads)
target_link_libraries(networking-communication-manager PRIVATE concurrency-tpool)
target_link_libraries(networking-communication-manager PRIVATE utilities)

#if (BUILD_TESTING)
#	add_subdirectory("test")
#endif()
