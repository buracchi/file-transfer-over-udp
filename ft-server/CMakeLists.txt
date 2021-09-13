project(
        "File transfer over UDP Server"
        VERSION 1.0.0
        LANGUAGES C
)

add_executable(
        ft-server
        "src/main.c"
)
target_link_libraries(ft-server PRIVATE ft-handler)
target_link_libraries(ft-server PRIVATE networking-communication-manager)
target_link_libraries(ft-server PRIVATE tproto-gbn)
target_link_libraries(ft-server PRIVATE utilities)
target_link_libraries(ft-server INTERFACE coverage_config)

add_library(
        ft-service
        STATIC
        "src/ft_service.h"
        "src/ft_service.c"
)
target_link_libraries(ft-service PRIVATE utilities)
target_link_libraries(ft-service INTERFACE coverage_config)

add_library(
        ft-handler
        STATIC
        "src/ft_handler.c"
        "src/ft_handler.h"
)
target_link_libraries(ft-handler PRIVATE ft-service)
target_link_libraries(ft-handler PRIVATE ftcp)
target_link_libraries(ft-handler PRIVATE concurrency-rwfslock)
target_link_libraries(ft-handler PRIVATE networking-request-handler)
target_link_libraries(ft-handler PRIVATE networking-socket2)
target_link_libraries(ft-handler PRIVATE utilities)
target_link_libraries(ft-handler INTERFACE coverage_config)

if (BUILD_TESTING)
    add_subdirectory("test")
endif ()
