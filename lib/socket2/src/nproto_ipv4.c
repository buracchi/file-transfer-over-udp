#include "nproto_ipv4.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

#include "try.h"
#include "utilities.h"

static struct nproto_ipv4_vtbl* get_nproto_ipv4_vtbl();
static void nproto_vtbl_init(struct nproto_vtbl** pvtbl);
static int destroy(struct nproto_ipv4* this);
static struct sockaddr2* get_sockaddr2(struct nproto* nproto, const char* url);

extern int nproto_ipv4_init(struct nproto_ipv4* this) {
    memset(this, 0, sizeof * this);
    nproto_vtbl_init(&this->super.nproto.__ops_vptr);
    this->__ops_vptr = get_nproto_ipv4_vtbl();
    this->super.nproto.domain = AF_INET;
    return 0;
}

static struct nproto_ipv4_vtbl* get_nproto_ipv4_vtbl() {
    struct nproto_ipv4_vtbl b_vtbl_zero = { 0 };
    if (!memcmp(&b_vtbl_zero, &__nproto_ipv4_ops_vtbl, sizeof * &__nproto_ipv4_ops_vtbl)) {
        __nproto_ipv4_ops_vtbl.destroy = destroy;
    }
    return &__nproto_ipv4_ops_vtbl;
}

static void nproto_vtbl_init(struct nproto_vtbl** pvtbl) {
    struct nproto_vtbl nproto_vtbl_zero = { 0 };
    if (!memcmp(&nproto_vtbl_zero, &__nproto_ops_vtbl, sizeof * &__nproto_ops_vtbl)) {
        __nproto_ops_vtbl.get_sockaddr2 = get_sockaddr2;    // override
    }
    *pvtbl = &__nproto_ops_vtbl;
}

static int destroy(struct nproto_ipv4* this) {
    return 0;
}

static struct sockaddr2* get_sockaddr2(struct nproto* nproto, const char* url) {
    struct nproto_ipv4* this = container_of(nproto, struct nproto_ipv4, super.nproto);
    struct sockaddr2* sockaddr2;
    struct sockaddr_in* paddr_in;
    struct in_addr haddr;
    char* buffer, * token, * saveptr;
    try(sockaddr2 = malloc(sizeof * sockaddr2), NULL, fail);
    try(paddr_in = malloc(sizeof * paddr_in), NULL, fail2);
    try(buffer = malloc(strlen(url) + 1), NULL, fail3);
    strcpy(buffer, url);

    uint16_t port;
    char* address;
    try(address = strtok_r(buffer, ":", &saveptr), NULL, fail4);
    try(str_to_uint16(strtok_r(NULL, ":", &saveptr), &port), 1, fail4);
    try(inet_aton(address, &haddr), 0, fail);
    free(buffer);

    memset(paddr_in, 0, sizeof * paddr_in);
    paddr_in->sin_family = AF_INET;
    paddr_in->sin_addr = haddr;
    paddr_in->sin_port = htons(port);
    sockaddr2->addr = (struct sockaddr*)paddr_in;
    sockaddr2->addrlen = sizeof * paddr_in;
    return sockaddr2;
fail4:
    free(buffer);
fail3:
    free(paddr_in);
fail2:
    free(sockaddr2);
fail:
    return NULL;
}
