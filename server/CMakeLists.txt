find_package(buracchi-argparser CONFIG REQUIRED)
find_package(buracchi-common CONFIG REQUIRED)
find_package(libevent CONFIG REQUIRED)

add_executable(server "src/main.c")
target_link_libraries(server PRIVATE $<TARGET_OBJECTS:server_obj>)
target_link_libraries(server PRIVATE buracchi::argparser::argparser)
target_link_libraries(server PRIVATE buracchi::common::logger)
target_link_libraries(server PRIVATE buracchi::common::utilities)
target_link_libraries(server PRIVATE libevent::core libevent::pthreads)
target_link_libraries(server PRIVATE network_utils)
target_link_libraries(server INTERFACE $<BUILD_INTERFACE:coverage_config>)

add_library(server_obj OBJECT
        "src/cmdline.c"
        "src/acceptor.c"
        "src/dispatcher.c"
        "src/service_handler.c"
        )
target_include_directories(server_obj PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>")
target_link_libraries(server_obj PRIVATE buracchi::argparser::argparser)
target_link_libraries(server_obj PRIVATE buracchi::common::logger)
target_link_libraries(server_obj PRIVATE buracchi::common::utilities)
target_link_libraries(server_obj PUBLIC libevent::core libevent::pthreads)
target_link_libraries(server_obj PRIVATE network_utils)
target_link_libraries(server_obj INTERFACE $<BUILD_INTERFACE:coverage_config>)

if (BUILD_TESTS)
    add_subdirectory("test")
endif ()
