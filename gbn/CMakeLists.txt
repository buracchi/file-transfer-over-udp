find_package(Libevent CONFIG REQUIRED)
find_package(buracchi-common CONFIG REQUIRED)

add_library(
	tproto-gbn
	STATIC
	"src/tproto_service_gbn.c"
	"src/gbn_statemachine.c"
)
target_include_directories(tproto-gbn PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(tproto-gbn PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(tproto-gbn PRIVATE "${LIBEVENT_INCLUDE_DIR}")
target_link_libraries(tproto-gbn PRIVATE libevent::core libevent::extra libevent::pthreads)
target_link_libraries(tproto-gbn PRIVATE buracchi::common::containers)
target_link_libraries(tproto-gbn PRIVATE buracchi::common::networking)
target_link_libraries(tproto-gbn PRIVATE buracchi::common::utilities)
target_link_libraries(tproto-gbn PRIVATE statemachine)

if (BUILD_TESTS)
	add_subdirectory("test")
endif()
