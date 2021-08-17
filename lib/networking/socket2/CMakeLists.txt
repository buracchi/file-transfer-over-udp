add_library(
	socket2
	"src/socket2.c"
)
target_compile_definitions(socket2 PRIVATE SOCKET2_VERSION=1)
target_include_directories(socket2 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(socket2 PRIVATE utilities)
target_link_libraries(socket2 INTERFACE coverage_config)

add_library(
	nproto-ipv4
	"src/nproto/nproto_service_ipv4.c"
)
target_compile_definitions(nproto-ipv4 PRIVATE NPROTO_IPV4_VERSION=1)
target_include_directories(nproto-ipv4 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(nproto-ipv4 PRIVATE utilities)
target_link_libraries(nproto-ipv4 INTERFACE coverage_config)

add_library(
	nproto-unix
	"src/nproto/nproto_service_unix.c"
)
target_compile_definitions(nproto-unix PRIVATE NPROTO_UNIX_VERSION=1)
target_include_directories(nproto-unix PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(nproto-unix PRIVATE utilities)
target_link_libraries(nproto-unix INTERFACE coverage_config)

add_library(
	tproto-tcp
	"src/tproto/tproto_service_tcp.c"
)
target_compile_definitions(tproto-tcp PRIVATE TPROTO_TCP_VERSION=1)
target_include_directories(tproto-tcp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tproto-tcp PRIVATE utilities)
target_link_libraries(tproto-tcp INTERFACE coverage_config)

add_library(
	tproto-udp
	"src/tproto/tproto_service_udp.c"
)
target_compile_definitions(tproto-udp PRIVATE TPROTO_UDP_VERSION=1)
target_include_directories(tproto-udp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(tproto-udp PRIVATE utilities)
target_link_libraries(tproto-udp INTERFACE coverage_config)

if (BUILD_TESTING)
	add_subdirectory("test")
endif()
