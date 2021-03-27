#include "nproto_unix.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/un.h> 

#include "try.h"
#include "utilities.h"

static struct nproto_unix_vtbl* get_nproto_unix_vtbl();
static void nproto_vtbl_init(struct nproto_vtbl** pvtbl);
static int destroy(struct nproto_unix* this);
static struct sockaddr2* get_sockaddr2(struct nproto* nproto, const char* url);

extern int nproto_unix_init(struct nproto_unix* this) {
    memset(this, 0, sizeof * this);
    nproto_vtbl_init(&this->super.nproto.__ops_vptr);
    this->__ops_vptr = get_nproto_unix_vtbl();
    this->super.nproto.domain = AF_UNIX;
    return 0;
}

static struct nproto_unix_vtbl* get_nproto_unix_vtbl() {
    struct nproto_unix_vtbl b_vtbl_zero = { 0 };
    if (!memcmp(&b_vtbl_zero, &__nproto_unix_ops_vtbl, sizeof * &__nproto_unix_ops_vtbl)) {
        __nproto_unix_ops_vtbl.destroy = destroy;
    }
    return &__nproto_unix_ops_vtbl;
}

static void nproto_vtbl_init(struct nproto_vtbl** pvtbl) {
    struct nproto_vtbl nproto_vtbl_zero = { 0 };
    if (!memcmp(&nproto_vtbl_zero, &__nproto_ops_vtbl, sizeof * &__nproto_ops_vtbl)) {
        __nproto_ops_vtbl.get_sockaddr2 = get_sockaddr2;    // override
    }
    *pvtbl = &__nproto_ops_vtbl;
}

static int destroy(struct nproto_unix* this) {
    return 0;
}

static struct sockaddr2* get_sockaddr2(struct nproto* nproto, const char* url) {
    struct nproto_unix* this = container_of(nproto, struct nproto_unix, super.nproto);
    struct sockaddr2* sockaddr2;
    struct sockaddr_un* paddr_un;
    const char* address = url;
    try(sockaddr2 = malloc(sizeof * sockaddr2), NULL, fail);
    try(paddr_un = malloc(sizeof * paddr_un), NULL, fail2);
    memset(paddr_un, 'x', sizeof * paddr_un);
    paddr_un->sun_family = AF_UNIX;
    paddr_un->sun_path[0] = '\0';
    strcpy(paddr_un->sun_path + 1, address);
    sockaddr2->addr = (struct sockaddr*)paddr_un;
    sockaddr2->addrlen = (socklen_t)(offsetof(struct sockaddr_un, sun_path) + 1 + strlen(address));
    return sockaddr2;
fail2:
    free(sockaddr2);
fail:
    return NULL;
}
