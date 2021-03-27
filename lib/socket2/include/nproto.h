#pragma once

#include "sockaddr2.h"

struct nproto {
    int domain;
    struct nproto_vtbl* __ops_vptr;
};

static struct nproto_vtbl {
    struct sockaddr2* (*get_sockaddr2)(struct nproto* nproto, const char* url);
} __nproto_ops_vtbl = { 0 };

static inline int nproto_get_domain(struct nproto* nproto) {
    return nproto->domain;
}

static inline struct sockaddr2* nproto_get_sockaddr2(struct nproto* nproto, const char* url) {
    return nproto->__ops_vptr->get_sockaddr2(nproto, url);
}
