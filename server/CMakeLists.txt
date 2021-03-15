cmake_minimum_required (VERSION 3.8)

project(
	"File transfer over UDP Server"
	VERSION 1.0.0
	LANGUAGES C
)

# add the executable

add_executable (
	server
	"main.c"
	"fts.h"
	"fts.c"
)

# add the libraries

find_package(Libevent CONFIG REQUIRED)

target_include_directories(server PRIVATE "${LIBEVENT_INCLUDE_DIR}")

target_link_libraries(server PRIVATE libevent::core libevent::extra libevent::pthreads)
target_link_libraries(server PUBLIC socket2)
target_link_libraries(server PUBLIC tpool)
target_link_libraries(server PUBLIC try)
target_link_libraries(server PUBLIC utilities)

# TODO: Aggiungere i test e, se necessario, installare le destinazioni.
