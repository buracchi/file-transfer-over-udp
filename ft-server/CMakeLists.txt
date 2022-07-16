find_package(buracchi-common CONFIG REQUIRED)

add_executable(
        ft-server
        "src/main.c"
)
target_link_libraries(ft-server PRIVATE ft-handler)
target_link_libraries(ft-server PRIVATE buracchi::common::argparser)
target_link_libraries(ft-server PRIVATE buracchi::common::networking)
target_link_libraries(ft-server PRIVATE buracchi::common::utilities)
#target_link_libraries(ft-server PRIVATE tproto-gbn)
target_link_libraries(ft-server INTERFACE coverage_config)

add_library(
        ft-service
        STATIC
        "src/ft_service.h"
        "src/ft_service.c"
)
target_link_libraries(ft-service PRIVATE buracchi::common::utilities)
target_link_libraries(ft-service INTERFACE coverage_config)

add_library(
        ft-handler
        STATIC
        "src/ft_handler.c"
        "src/ft_handler.h"
)
target_link_libraries(ft-handler PRIVATE ft-service)
target_link_libraries(ft-handler PRIVATE ftcp)
target_link_libraries(ft-handler PRIVATE buracchi::common::concurrency)
target_link_libraries(ft-handler PRIVATE buracchi::common::networking)
target_link_libraries(ft-handler PRIVATE buracchi::common::utilities)
target_link_libraries(ft-handler INTERFACE coverage_config)

if (BUILD_TESTING)
    add_subdirectory("test")
endif ()
