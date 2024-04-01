#include "acceptor.h"

#include <buracchi/common/logger/logger.h>
#include <buracchi/common/utilities/try.h>

#include "inet_utils.h"

static ssize_t recv_packet(evutil_socket_t sockfd, struct addrinfo receiver_addrinfo[static 1], struct acceptor_packet packet[static 1]);

static struct evutil_addrinfo *get_accepting_addrinfo_list(char const *service);

static int attempt_binding(evutil_socket_t *sockfd, struct addrinfo *addrinfo);

static evutil_socket_t create_client_transport_endpoint(struct acceptor_packet *packet);

int acceptor_init(struct acceptor acceptor[static 1], char const *port) {
    *acceptor = (struct acceptor) {
        .peer_acceptor = EVUTIL_INVALID_SOCKET,
    };
    struct evutil_addrinfo *accepting_addrinfo_list;
    try(accepting_addrinfo_list = get_accepting_addrinfo_list(port), nullptr, get_accepting_addrinfo_list_fail);
    for (struct addrinfo *addrinfo = accepting_addrinfo_list; addrinfo != nullptr; addrinfo = addrinfo->ai_next) {
        if (attempt_binding(&acceptor->peer_acceptor, addrinfo)) {
            continue;
        }
        memcpy(&acceptor->addr_storage, addrinfo->ai_addr, addrinfo->ai_addrlen);
        memcpy(&acceptor->addrinfo, addrinfo, sizeof acceptor->addrinfo);
        acceptor->addrinfo.ai_addr = (struct sockaddr *)&acceptor->addr_storage;
        break;
    }
    evutil_freeaddrinfo(accepting_addrinfo_list);
    if (acceptor->peer_acceptor == EVUTIL_INVALID_SOCKET) {
        cmn_logger_log_error("Address already in use.");
        goto fail;
    }
    try(evutil_make_socket_nonblocking(acceptor->peer_acceptor), -1, evutil_make_socket_nonblocking_fail);
    return 0;
evutil_make_socket_nonblocking_fail:
    cmn_logger_log_error("evutil_make_socket_nonblocking_fail: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
    try(evutil_closesocket(acceptor->peer_acceptor), -1, close_fail);
    acceptor->peer_acceptor = EVUTIL_INVALID_SOCKET;
    goto fail;
close_fail:
    cmn_logger_log_error("close: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
    acceptor->peer_acceptor = EVUTIL_INVALID_SOCKET;
    goto fail;
get_accepting_addrinfo_list_fail:
    goto fail;
fail:
	cmn_logger_log_error("Can't create a passive mode transport endpoint.");
	return 1;
}

int acceptor_destroy(struct acceptor acceptor[static 1]) {
	if (evutil_closesocket(acceptor->peer_acceptor) == -1) {
        cmn_logger_log_error("close: %s", evutil_socket_error_to_string(evutil_socket_geterror(acceptor->peer_acceptor)));
        return 1;
    }
	return 0;
}

bool acceptor_accept(struct acceptor acceptor[static 1], evutil_socket_t socket[static 1], struct acceptor_packet packet[static 1]) {
    char client_address_str[INET6_ADDRSTRLEN] = {};
    uint16_t client_address_port;
    char rbuff_hex[sizeof packet->data] = {};
    if (recv_packet(acceptor->peer_acceptor, &acceptor->addrinfo, packet) == -1) {
        cmn_logger_log_debug("Acceptor: Unable to receive the request packet correctly, denying connection.");
        return false;
    }
    if (inet_ntop_address(packet->src_addrinfo.ai_addr, client_address_str, &client_address_port) == nullptr) {
        cmn_logger_log_error("inet_ntop_address failed");
        return false;
    }
    for (ssize_t i = 0; i < packet->payload_size; i++) {
        sprintf(rbuff_hex + 2 * i, "%02x", packet->data[i]);
    }
    cmn_logger_log_debug("Acceptor: Got packet from host %s on port %hu of %zu bytes, packet content is \"%s\"",
                         client_address_str, client_address_port, packet->payload_size, rbuff_hex);
    *socket = create_client_transport_endpoint(packet);
    if (*socket == EVUTIL_INVALID_SOCKET) {
        cmn_logger_log_error("Acceptor: Unable to create an endpoint to the client, denying connection.");
        return false;
        }
    return true;
}

// Resolves the specified service and returns a list of addrinfo
// structures containing information about the resulting addresses suitable for
// binding a socket that will accept connections.
// On error, returns nullptr.
static struct evutil_addrinfo *get_accepting_addrinfo_list(char const *service) {
	struct evutil_addrinfo *result = nullptr;
	struct evutil_addrinfo hints = {
		.ai_flags = EVUTIL_AI_PASSIVE | EVUTIL_AI_NUMERICSERV | EVUTIL_AI_ADDRCONFIG,
		.ai_family = AF_INET6,
		.ai_socktype = SOCK_DGRAM,
		.ai_protocol = 0,
		.ai_addrlen = 0,
		.ai_addr = nullptr,
		.ai_canonname = nullptr,
		.ai_next = nullptr
	};
	int gai_ret;
	if ((gai_ret = evutil_getaddrinfo(nullptr, service, &hints, &result))) {
        cmn_logger_log_error("getaddrinfo: %s", gai_strerror(gai_ret));
		if (result) {
            // According to the POSIX 2008 standard, if getaddrinfo returns
            // a non-zero error value, the content of the result parameter
            // is undefined.
            // Therefore, it is not guaranteed that the result parameter will
            // not be assigned in the event of a getaddrinfo failure.
			freeaddrinfo(result);
		}
		result = nullptr;
	}
	return result;
}

// Attempts to bind a socket that will accept connections using the given
// address info structure
// Returns 0 on success, 1 on failure
static int attempt_binding(evutil_socket_t *sockfd, struct addrinfo *addrinfo) {
	char bind_addr_str[INET6_ADDRSTRLEN] = { 0 };
	uint16_t bind_addr_port;
	try(*sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol), EVUTIL_INVALID_SOCKET, socket_fail);
	try(evutil_make_listen_socket_reuseable(*sockfd), -1, evutil_make_listen_socket_reuseable_fail);
	try(setsockopt(*sockfd, IPPROTO_IP, IP_RECVORIGDSTADDR, &(int){ true }, sizeof(int)) == -1, true, setsockopt_fail);
	try(setsockopt(*sockfd, IPPROTO_IPV6, IPV6_RECVORIGDSTADDR, &(int){ true }, sizeof(int)) == -1, true, setsockopt_fail);
	try(setsockopt(*sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &(int){ false }, sizeof(int)) == -1, true, setsockopt_fail);
	try(inet_ntop_address(addrinfo->ai_addr, bind_addr_str, &bind_addr_port), nullptr, inet_ntop_address_fail);
	cmn_logger_log_debug("Binding to host %s on port %hu", bind_addr_str, bind_addr_port);
	if (bind(*sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1) {
		cmn_logger_log_debug("bind: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
		try(evutil_closesocket(*sockfd), -1, close_fail);
		return 1;
	}
	cmn_logger_log_info("Acceptor bound to host %s on port %hu", bind_addr_str, bind_addr_port);
	return 0;
socket_fail:
	cmn_logger_log_debug("socket: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
	return 1;
evutil_make_listen_socket_reuseable_fail:
	cmn_logger_log_error("evutil_make_listen_socket_reuseable: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
	try(evutil_closesocket(*sockfd), -1, close_fail);
	*sockfd = EVUTIL_INVALID_SOCKET;
	return 1;
setsockopt_fail:
	cmn_logger_log_error("setsockopt: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
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

static evutil_socket_t create_client_transport_endpoint(struct acceptor_packet *packet) {
	evutil_socket_t sockfd;
	char src_addr_str[INET6_ADDRSTRLEN] = { 0 };
	char dest_addr_str[INET6_ADDRSTRLEN] = { 0 };
	uint16_t client_address_port;
	uint16_t request_destination_address_port;
	try(inet_ntop_address(packet->src_addrinfo.ai_addr, src_addr_str, &client_address_port), nullptr, inet_ntop_address_fail);
	try(inet_ntop_address(packet->dest_addrinfo.ai_addr, dest_addr_str, &request_destination_address_port), nullptr, inet_ntop_address_fail);
	cmn_logger_log_debug("Acceptor: Creating connection from %s on port %hu to host %s on port %hu", dest_addr_str, request_destination_address_port, src_addr_str, client_address_port);
	try(sockfd = socket(packet->dest_addrinfo.ai_family, SOCK_DGRAM, 0), EVUTIL_INVALID_SOCKET, socket_fail);
	try(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &(int){ true }, sizeof(int)) == -1, true, setsockopt_fail);
	try(bind(sockfd, packet->dest_addrinfo.ai_addr, packet->dest_addrinfo.ai_addrlen), -1, bind_fail);
	try(connect(sockfd, packet->src_addrinfo.ai_addr, packet->src_addrinfo.ai_addrlen), -1, connect_fail);
	if (packet->dest_addrinfo.ai_protocol == IPPROTO_IP) {
		cmn_logger_log_debug("Acceptor: Setting connection network protocol to IPV4 to support IPV6 unaware clients");
		try(setsockopt(sockfd, IPPROTO_IPV6, IPV6_ADDRFORM, &(int){ AF_INET }, sizeof(int)) == -1, true, setsockopt_fail);
	}
	try(evutil_make_socket_nonblocking(sockfd), -1, evutil_make_socket_nonblocking_fail);
	print_inet_address_detail(sockfd);
	return sockfd;
inet_ntop_address_fail:
	cmn_logger_log_error("inet_ntop_address failed");
	return EVUTIL_INVALID_SOCKET;
socket_fail:
	cmn_logger_log_error("socket: %s", evutil_socket_error_to_string(evutil_socket_geterror(client_socket)));
	return EVUTIL_INVALID_SOCKET;
setsockopt_fail:
	cmn_logger_log_error("setsockopt: %s", evutil_socket_error_to_string(evutil_socket_geterror(client_socket)));
	return EVUTIL_INVALID_SOCKET;
bind_fail:
	cmn_logger_log_error("bind: %s", evutil_socket_error_to_string(evutil_socket_geterror(client_socket)));
	return EVUTIL_INVALID_SOCKET;
connect_fail:
	cmn_logger_log_error("connect: %s", evutil_socket_error_to_string(evutil_socket_geterror(client_socket)));
	return EVUTIL_INVALID_SOCKET;
evutil_make_socket_nonblocking_fail:
	cmn_logger_log_error("evutil_make_socket_nonblocking_fail: %s", evutil_socket_error_to_string(evutil_socket_geterror(peer_acceptor)));
	return EVUTIL_INVALID_SOCKET;
}

static ssize_t recv_packet(evutil_socket_t sockfd, struct addrinfo receiver_addrinfo[static 1], struct acceptor_packet packet[static 1]) {
	ssize_t bytes_recv;
	char *msg_control = nullptr;
	size_t msg_control_size = 128;
	try(msg_control = calloc(1, msg_control_size), nullptr, msg_control_calloc_fail);
	struct iovec iovec[1] = {
		{.iov_base = packet->data, .iov_len = sizeof packet->data}
	};
    *packet = (struct acceptor_packet) {
        .src_addrinfo = { .ai_addr = (struct sockaddr *)&packet->src_addr_storage,  .ai_addrlen = sizeof packet->src_addr_storage},
        .dest_addrinfo = {.ai_addr = (struct sockaddr *)&packet->dest_addr_storage, .ai_addrlen = sizeof packet->dest_addr_storage}
    };
	struct msghdr msghdr = {
		.msg_name = packet->src_addrinfo.ai_addr,
		.msg_namelen = packet->src_addrinfo.ai_addrlen,
		.msg_iov = iovec,
		.msg_iovlen = (sizeof iovec) / (sizeof *iovec),
		.msg_control = msg_control,
		.msg_controllen = sizeof msg_control,
		.msg_flags = MSG_CTRUNC
	};
	(void)recvmsg(sockfd, &msghdr, MSG_PEEK | MSG_TRUNC);
	if (msghdr.msg_flags & MSG_TRUNC) {
		cmn_logger_log_debug("Acceptor: request packet payload exceeded maximum allowed size, denying connection.");
		return -1;
	}
	while (msghdr.msg_flags & MSG_CTRUNC) {
		if (msg_control_size == 4096) {
			cmn_logger_log_debug("Acceptor: msg_control buffer exceed 4096 bytes, denying connection.");
			return -1;
		}
		char *msg_control_reallocd = nullptr;
		msg_control_size <<= 1;
		try(msg_control_reallocd = realloc(msg_control, msg_control_size), nullptr, msg_control_realloc_fail);
		msg_control = msg_control_reallocd;
		memset(msg_control, 0, msg_control_size);
		msghdr.msg_control = msg_control;
		msghdr.msg_controllen = msg_control_size;
		(void)recvmsg(sockfd, &msghdr, MSG_PEEK | MSG_TRUNC);
	}
	bytes_recv = recvmsg(sockfd, &msghdr, 0);
	if (msghdr.msg_flags & MSG_CTRUNC || msghdr.msg_flags & MSG_TRUNC) {
		cmn_logger_log_debug("Acceptor: request packet read was truncated, denying connection.");
		return -1;
	}
    packet->src_addrinfo.ai_family = packet->src_addrinfo.ai_addr->sa_family;
	for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msghdr); cmsg != nullptr; cmsg = CMSG_NXTHDR(&msghdr, cmsg)) {
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_ORIGDSTADDR) {
			struct sockaddr_in6 addr;
			try(inet_addr_in_to_in6((struct sockaddr_in *)CMSG_DATA(cmsg), &addr), -1, inet_addr_in_to_in6_fail);
			memcpy(packet->dest_addrinfo.ai_addr, &addr, sizeof addr);
			packet->dest_addrinfo.ai_addrlen = sizeof addr;
			packet->dest_addrinfo.ai_family = addr.sin6_family;
			packet->dest_addrinfo.ai_protocol = IPPROTO_IP;
			break;
		}
		if (cmsg->cmsg_level == IPPROTO_IPV6 && cmsg->cmsg_type == IPV6_ORIGDSTADDR) {
			struct sockaddr_in6 *addr = (struct sockaddr_in6 *)CMSG_DATA(cmsg);
			memcpy(packet->dest_addrinfo.ai_addr, addr, sizeof *addr);
			packet->dest_addrinfo.ai_addrlen = sizeof *addr;
			packet->dest_addrinfo.ai_family = addr->sin6_family;
			packet->dest_addrinfo.ai_protocol = IPPROTO_IPV6;
			break;
		}
	}
	free(msg_control);
	if (!memcmp(&packet->dest_addr_storage, &(struct sockaddr_storage){ 0 }, sizeof packet->dest_addr_storage)) {
		cmn_logger_log_debug("Acceptor: no IP control messages were included in the packet header. "
		                     "Using the acceptor address as the packet destination address.");
		memcpy(&packet->dest_addrinfo, receiver_addrinfo, sizeof packet->dest_addrinfo);
	}
    packet->payload_size = bytes_recv;
	return bytes_recv;
inet_addr_in_to_in6_fail:
	cmn_logger_log_error("inet_addr_in_to_in6 failed");
	return -1;
msg_control_calloc_fail:
	cmn_logger_log_error("calloc: %s", strerror(errno));
	return -1;
msg_control_realloc_fail:
	cmn_logger_log_error("realloc: %s", strerror(errno));
	free(msg_control);
	return -1;
}
