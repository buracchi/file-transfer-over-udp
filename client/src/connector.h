#pragma once

#include <event2/util.h>

/**
 * Connects to a server at the specified host and port.
 * The sockfd parameter must be set to EVUTIL_INVALID_SOCKET before calling this function.
 * On success, sockfd will be set to the socket descriptor of the connection.
 * On failure, sockfd will be left unchanged.
 *
 * @param sockfd a pointer to the socket descriptor to set
 * @param host the hostname or IP address of the server
 * @param port the port number of the server
 * @return 0 on success, 1 on failure.
 */
extern int connector_connect(evutil_socket_t *sockfd, char const *host, char const *port);
