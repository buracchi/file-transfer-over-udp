cmake_minimum_required (VERSION 3.8)

project(
	"File transfer over UDP Client"
	VERSION 1.0.0
	LANGUAGES C
)

# add the executable

add_executable (
	client
	"src/main.c"
)

# add shared libraries

target_link_libraries(client PRIVATE socket2)
target_link_libraries(client PRIVATE try)
target_link_libraries(client PRIVATE utilities)

# TODO: Aggiungere i test e, se necessario, installare le destinazioni.
