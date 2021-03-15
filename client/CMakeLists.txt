cmake_minimum_required (VERSION 3.8)

project(
	"File transfer over UDP Client"
	VERSION 1.0.0
	LANGUAGES C
)

# add the executable

add_executable (
	client
	"main.c"
)

# add shared librareis

target_link_libraries(client PUBLIC socket2)
target_link_libraries(client PUBLIC try)
target_link_libraries(client PUBLIC utilities)

# TODO: Aggiungere i test e, se necessario, installare le destinazioni.
