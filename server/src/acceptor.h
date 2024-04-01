#pragma once

#include <event2/util.h>

struct acceptor {
	struct sockaddr_storage addr_storage; // should not be directly accessed
    struct evutil_addrinfo addrinfo;
	evutil_socket_t peer_acceptor;
};

struct acceptor_packet {
    struct sockaddr_storage src_addr_storage;  // should not be directly accessed
    struct sockaddr_storage dest_addr_storage; // should not be directly accessed

    struct evutil_addrinfo src_addrinfo;
    struct evutil_addrinfo dest_addrinfo;
    char data[512];
    ssize_t payload_size;
};

/**
 * Initializes an acceptor object listening to incoming connections on the specified port.
 *
 * @param acceptor pointer to the acceptor struct to be initialized
 * @param port the port number on which to listen for connections
 * @return 0 on success, non-zero on failure
 */
int acceptor_init(struct acceptor acceptor[static 1], char const *port);

/**
 * Releases resources associated with an acceptor struct
 *
 * @param acceptor pointer to the acceptor struct to be destroyed
 * @return 0 on success, non-zero on failure
 */
int acceptor_destroy(struct acceptor acceptor[static 1]);

/**
 * Accept an incoming request providing the socket for the connection and the received packet.
 *
 * @param acceptor
 * @param socket
 * @param packet
 */
bool acceptor_accept(struct acceptor acceptor[static 1], evutil_socket_t socket[static 1], struct acceptor_packet packet[static 1]);
