add_subdirectory(argparser)
add_subdirectory(misc)
add_subdirectory(try)

add_library(utilities INTERFACE)
target_compile_definitions(utilities INTERFACE UTILITIES_VERSION=1)
target_link_libraries(utilities INTERFACE utilities-argparser)
target_link_libraries(utilities INTERFACE utilities-misc)
target_link_libraries(utilities INTERFACE utilities-try)
