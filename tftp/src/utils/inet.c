#include "inet.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

const char *sockaddr_ntop(const struct sockaddr address[static 1], char address_str[static INET6_ADDRSTRLEN], uint16_t port[static 1]) {
	void *client_addr;
	switch (address->sa_family) {
	case AF_INET: {
        struct sockaddr_in *ipv4_client_addr;
        ipv4_client_addr = (struct sockaddr_in *) address;
        client_addr = &(ipv4_client_addr->sin_addr);
        *port = ntohs(ipv4_client_addr->sin_port);
    }
		break;
	case AF_INET6: {
        struct sockaddr_in6 *ipv6_client_addr;
        ipv6_client_addr = (struct sockaddr_in6 *) address;
        client_addr = &(ipv6_client_addr->sin6_addr);
        *port = ntohs(ipv6_client_addr->sin6_port);
    }
		break;
	default:
		// SET ERRNO
		return nullptr;
	}
	return inet_ntop(address->sa_family, client_addr, address_str, INET6_ADDRSTRLEN);
}

int sockaddr_in_to_in6(const struct sockaddr_in src[static 1], struct sockaddr_in6 dest[static 1]) {
	char host[INET_ADDRSTRLEN] = { 0 };
	char serv[6] = { 0 }; // 0 to 65536 plus '\0'
	struct addrinfo *addrinfo;
    static const struct addrinfo hints = {
		.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG,
		.ai_family = AF_INET6,
		.ai_socktype = SOCK_DGRAM,
		.ai_protocol = 0,
		.ai_addrlen = 0,
		.ai_addr = nullptr,
		.ai_canonname = nullptr,
		.ai_next = nullptr
	};
	int gai_ret;
	gai_ret = getnameinfo((const struct sockaddr *)src, sizeof *src, host, sizeof host, serv, sizeof serv, NI_DGRAM | NI_NUMERICSERV | NI_NUMERICHOST);
	if (gai_ret != 0) {
		return gai_ret;
	}
	gai_ret = getaddrinfo(host, serv, &hints, &addrinfo);
	if (gai_ret != 0) {
		return gai_ret;
	}
	memcpy(dest, addrinfo->ai_addr, sizeof *dest);
	freeaddrinfo(addrinfo);
	return 0;
}

int sockaddr_in6_to_in(const struct sockaddr_in6 src[static 1], struct sockaddr_in dest[static 1]) {
    char host[INET6_ADDRSTRLEN] = { 0 };
    char serv[6] = { 0 }; // 0 to 65536 plus '\0'
    struct addrinfo *addrinfo;
    static const struct addrinfo hints = {
        .ai_flags = AI_ADDRCONFIG,
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
        .ai_protocol = 0,
        .ai_addrlen = 0,
        .ai_addr = nullptr,
        .ai_canonname = nullptr,
        .ai_next = nullptr
    };
    int gai_ret;
    gai_ret = getnameinfo((const struct sockaddr *)src, sizeof *src, host, sizeof host, serv, sizeof serv, NI_DGRAM | NI_NUMERICSERV | NI_NUMERICHOST);
    if (gai_ret != 0) {
        return gai_ret;
    }
    if (IN6_IS_ADDR_UNSPECIFIED(&src->sin6_addr)) {
        dest->sin_family = AF_INET;
        dest->sin_addr.s_addr = htonl(INADDR_ANY);
        dest->sin_port = src->sin6_port;
        return 0;
    }
    if (IN6_IS_ADDR_V4MAPPED(&src->sin6_addr)) {
        struct in_addr addr4;
        memcpy(&addr4, &src->sin6_addr.s6_addr[12], sizeof(addr4));
        dest->sin_family = AF_INET;
        dest->sin_addr = addr4;
        dest->sin_port = src->sin6_port;
        return 0;
    }
    gai_ret = getaddrinfo(host, serv, &hints, &addrinfo);
    if (gai_ret != 0) {
        return gai_ret;
    }
    memcpy(dest, addrinfo->ai_addr, sizeof *dest);
    freeaddrinfo(addrinfo);
    return 0;
}

bool sockaddr_equals(const struct sockaddr_storage lhs[static 1], const struct sockaddr_storage rhs[static 1]) {
    if (lhs->ss_family != rhs->ss_family) {
        return false;
    }
    if (lhs->ss_family == AF_INET) {
        const struct sockaddr_in *lhs_in = (const struct sockaddr_in *) lhs;
        const struct sockaddr_in *rhs_in = (const struct sockaddr_in *) rhs;
        return lhs_in->sin_port == rhs_in->sin_port
               && lhs_in->sin_addr.s_addr == rhs_in->sin_addr.s_addr;
    }
    if (lhs->ss_family == AF_INET6) {
        const struct sockaddr_in6 *lhs_in6 = (const struct sockaddr_in6 *) lhs;
        const struct sockaddr_in6 *rhs_in6 = (const struct sockaddr_in6 *) rhs;
        return lhs_in6->sin6_port == rhs_in6->sin6_port
               && memcmp(&lhs_in6->sin6_addr, &rhs_in6->sin6_addr, sizeof(struct in6_addr)) == 0
               && lhs_in6->sin6_scope_id == rhs_in6->sin6_scope_id;
    }
    return false; // unsupported address family
}
