#pragma once

#include "nproto.h"

struct nproto_unix {
    struct {
        struct nproto nproto;
    } super;
    struct nproto_unix_vtbl* __ops_vptr;
};

static struct nproto_unix_vtbl {
    int (*destroy)(struct nproto_unix* nproto);
} __nproto_unix_ops_vtbl = { 0 };

extern int nproto_unix_init(struct nproto_unix* nproto);

static inline int nproto_unix_destroy(struct nproto_unix* nproto) {
    return nproto->__ops_vptr->destroy(nproto);
}
