#pragma once

#include "nproto.h"

struct nproto_ipv4 {
    struct {
        struct nproto nproto;
    } super;
    struct nproto_ipv4_vtbl* __ops_vptr;
};

static struct nproto_ipv4_vtbl {
    int (*destroy)(struct nproto_ipv4* nproto);
} __nproto_ipv4_ops_vtbl = { 0 };

extern int nproto_ipv4_init(struct nproto_ipv4* nproto);

static inline int nproto_ipv4_destroy(struct nproto_ipv4* nproto) {
    return nproto->__ops_vptr->destroy(nproto);
}
