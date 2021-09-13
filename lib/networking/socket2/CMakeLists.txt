add_library(
        nproto
        "src/nproto_service.c"
)
target_include_directories(nproto PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(nproto INTERFACE coverage_config)

add_library(
        tproto
        "src/tproto_service.c"
)
target_include_directories(tproto PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tproto INTERFACE coverage_config)

add_library(
        networking-socket2
        "src/socket2.c"
)
target_include_directories(networking-socket2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(networking-socket2 PUBLIC nproto)
target_link_libraries(networking-socket2 PUBLIC tproto)
target_link_libraries(networking-socket2 PRIVATE utilities)
target_link_libraries(networking-socket2 INTERFACE coverage_config)

add_library(
        nproto-ipv4
        "src/nproto/nproto_service_ipv4.c"
)
target_include_directories(nproto-ipv4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(nproto-ipv4 PUBLIC nproto)
target_link_libraries(nproto-ipv4 PRIVATE utilities)
target_link_libraries(nproto-ipv4 INTERFACE coverage_config)

add_library(
        nproto-unix
        "src/nproto/nproto_service_unix.c"
)
target_include_directories(nproto-unix PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(nproto-unix PUBLIC nproto)
target_link_libraries(nproto-unix PRIVATE utilities)
target_link_libraries(nproto-unix INTERFACE coverage_config)

add_library(
        tproto-tcp
        "src/tproto/tproto_service_tcp.c"
)
target_include_directories(tproto-tcp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tproto-tcp PRIVATE utilities)
target_link_libraries(tproto-tcp INTERFACE coverage_config)

if (BUILD_TESTING)
    add_subdirectory("test")
endif ()
