#include "connector.h"

#include <buracchi/common/logger/logger.h>
#include <buracchi/common/utilities/try.h>

#include "inet_utils.h"

static struct evutil_addrinfo *get_server_addrinfo_list(char const *node, char const *service);
static int attempt_connection(evutil_socket_t *sockfd, struct addrinfo *addrinfo);

// Attempts to connect to a server at the specified host and port
// Returns 0 on success, 1 on failure
extern int connector_connect(evutil_socket_t *sockfd, char const *host, char const *port) {
	struct evutil_addrinfo *server_addrinfo_list;
	cmn_logger_log_info("Establishing server connection.");
	try(server_addrinfo_list = get_server_addrinfo_list(host, port), NULL, fail);
	for (struct addrinfo *addrinfo = server_addrinfo_list; addrinfo != NULL; addrinfo = addrinfo->ai_next) {
		if (attempt_connection(sockfd, addrinfo)) {
			continue;
		}
		break;
	}
	freeaddrinfo(server_addrinfo_list);
	if (*sockfd == EVUTIL_INVALID_SOCKET) {
		cmn_logger_log_error("Connection to the server refused.");
		return 1;
	}
	print_inet_address_detail(*sockfd);
	return 0;
fail:
	return 1;
}

// Resolves the specified node and service and returns a list of addrinfo
// structures containing information about the resulting addresses.
// On error, returns NULL.
static struct evutil_addrinfo *get_server_addrinfo_list(char const *node, char const *service) {
	struct evutil_addrinfo *result = NULL;
	struct evutil_addrinfo hints = {
		.ai_flags = AI_NUMERICSERV | AI_ADDRCONFIG | AI_V4MAPPED,
		.ai_family = AF_INET6,
		.ai_socktype = SOCK_DGRAM,
		.ai_protocol = 0,
		.ai_addrlen = 0,
		.ai_addr = NULL,
		.ai_canonname = NULL,
		.ai_next = NULL
	};
	int gai_ret;
	if ((gai_ret = getaddrinfo(node, service, &hints, &result))) {
		// According to the POSIX 2008 standard, if getaddrinfo returns
		// a non-zero error value, the content of the res parameter
		// is undefined.
		// Therefore, it is not guaranteed that the res parameter will
		// not be assigned in the event of a getaddrinfo failure.
		cmn_logger_log_error("getaddrinfo: %s", gai_strerror(gai_ret));
		if (result) {
			freeaddrinfo(result);
		}
		result = NULL;
	}
	return result;
}

// Attempts to establish a connection using the given address info structure
// Returns 0 on success, 1 on failure
static int attempt_connection(evutil_socket_t *sockfd, struct addrinfo *addrinfo) {
	char server_addr_str[INET6_ADDRSTRLEN] = { 0 };
	uint16_t server_addr_port;
	struct sockaddr_storage sock_addr;
	socklen_t sock_addr_length = sizeof sock_addr;
	char sock_addr_str[INET6_ADDRSTRLEN] = { 0 };
	uint16_t sock_port;
	try(*sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol), EVUTIL_INVALID_SOCKET, socket_fail);
	try(inet_ntop_address(addrinfo->ai_addr, server_addr_str, &server_addr_port), NULL, inet_ntop_address_fail);
	cmn_logger_log_debug("Connecting to %s on port %hu", server_addr_str, server_addr_port);
	try(connect(*sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen), -1, connect_fail);
	getsockname(*sockfd, (struct sockaddr *)&sock_addr, &sock_addr_length);
	try(inet_ntop_address((struct sockaddr *)&sock_addr, sock_addr_str, &sock_port), NULL, inet_ntop_address_fail);
	cmn_logger_log_debug("Connected from host %s on port %hu", sock_addr_str, sock_port);
	return 0;
socket_fail:
	cmn_logger_log_debug("socket: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
	return 1;
connect_fail:
	cmn_logger_log_debug("connect: %s.", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
	try(evutil_closesocket(*sockfd), -1, close_fail);
	*sockfd = EVUTIL_INVALID_SOCKET;
	return 1;
inet_ntop_address_fail:
	cmn_logger_log_error("inet_ntop_address failed");
	try(evutil_closesocket(*sockfd), -1, close_fail);
	*sockfd = EVUTIL_INVALID_SOCKET;
	return 1;
close_fail:
	cmn_logger_log_error("close: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
	return 1;
}
