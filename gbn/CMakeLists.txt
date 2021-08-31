find_package(Libevent CONFIG REQUIRED)

add_library(
	tproto-gbnd
	STATIC
	"src/tproto_service_gbn.c"
)
target_include_directories(tproto-gbnd PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(tproto-gbnd PRIVATE "${LIBEVENT_INCLUDE_DIR}")
target_link_libraries(tproto-gbnd PRIVATE pthread)
target_link_libraries(tproto-gbnd PRIVATE socket2)
target_link_libraries(tproto-gbnd PRIVATE tproto)
target_link_libraries(tproto-gbnd PRIVATE nproto)
target_link_libraries(tproto-gbnd PRIVATE libevent::core libevent::extra libevent::pthreads)
target_link_libraries(tproto-gbnd PRIVATE try)
target_link_libraries(tproto-gbnd PRIVATE utilities)
