find_package(CLI11 CONFIG REQUIRED)

add_executable(client
    client.c
    client_cli.cpp
    client_packet_loss.c)
target_link_libraries(client
    PRIVATE logger
    PRIVATE tftp
    PRIVATE CLI11::CLI11)
target_link_options(client PRIVATE
    -Wl,--wrap=recvfrom
    -Wl,--wrap=sendto)
set_target_properties(client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/client"
    RUNTIME_OUTPUT_NAME "client")

add_executable(server
    server.c
    server_cli.cpp
    server_packet_loss.c)
target_include_directories(server PRIVATE $<TARGET_PROPERTY:tftp,SOURCE_DIR>/src/server)
target_link_libraries(server
    PRIVATE logger
    PRIVATE tftp
    PRIVATE CLI11::CLI11)
target_link_options(server PRIVATE
    -Wl,--wrap=dispatcher_submit_recvmsg
    -Wl,--wrap=sendto)
set_target_properties(server PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/server"
    RUNTIME_OUTPUT_NAME "server")
