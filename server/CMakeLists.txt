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
target_link_libraries(server PRIVATE pthread)
target_link_libraries(server PRIVATE socket2)
target_link_libraries(server PRIVATE tpool)
target_link_libraries(server PRIVATE try)
target_link_libraries(server PRIVATE utilities)

# TODO: Aggiungere i test e, se necessario, installare le destinazioni.
