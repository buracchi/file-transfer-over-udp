find_package(buracchi-common CONFIG REQUIRED)

add_library(
        statemachine
        STATIC
        "src/statemachine.c"
)
target_compile_definitions(statemachine PRIVATE STATEMACHINE_VERSION=1)
target_include_directories(statemachine PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(statemachine PRIVATE buracchi::common::utilities)
target_link_libraries(statemachine INTERFACE coverage_config)

if (BUILD_TESTING)
    add_subdirectory("test")
endif ()
