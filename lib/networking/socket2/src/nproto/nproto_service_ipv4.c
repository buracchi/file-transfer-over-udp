#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#include "types/nproto/nproto_service_ipv4.h"
#include "types/socket2.h"
#include "try.h"
#include "utilities.h"

static int set_address(cmn_nproto_service_t service, cmn_socket2_t socket, const char *url);

static int parse_url(const char *url, struct in_addr *haddr_in, uint16_t *port);

static struct cmn_nproto_service_vtbl __cmn_nproto_service_ops_vtbl = {.set_address = set_address};

static struct cmn_nproto_service_ipv4 service = {
        .super = {
                .__ops_vptr = &__cmn_nproto_service_ops_vtbl,
                .domain = AF_INET
        }
};

#ifdef WIN32 // May the gcc guy who forgot to create a flag for this stupid warning be cursed
extern cmn_nproto_service_t cmn_nproto_service_ipv4 = &(service.super);
#elif __unix__
cmn_nproto_service_t cmn_nproto_service_ipv4 = &(service.super);
#endif

static int set_address(cmn_nproto_service_t service, cmn_socket2_t socket, const char *url) {
    struct sockaddr_in *saddr_in;
    struct in_addr haddr_in;
    uint16_t port;
    try(saddr_in = malloc(sizeof *saddr_in), NULL, fail);
    memset(saddr_in, 0, sizeof *saddr_in);
    try(parse_url(url, &haddr_in, &port), 1, fail2);
    saddr_in->sin_family = AF_INET;
    saddr_in->sin_addr = haddr_in;
    saddr_in->sin_port = htons(port);
    socket->address = (struct sockaddr *) saddr_in;
    socket->addrlen = sizeof *saddr_in;
    return 0;
fail2:
    free(saddr_in);
fail:
    return 1;
}

static int parse_url(const char *url, struct in_addr *haddr_in, uint16_t *port) {
    char *buffer;
    char *saveptr;
    try(buffer = malloc(strlen(url) + 1), NULL, fail);
    strcpy(buffer, url);
    try(strtok_r(buffer, ":", &saveptr), NULL, fail2);
    try(str_to_uint16(strtok_r(NULL, ":", &saveptr), port), 1, fail2);
    try(inet_aton(buffer, haddr_in), 0, fail2);
    free(buffer);
    return 0;
fail2:
    free(buffer);
fail:
    return 1;
}
