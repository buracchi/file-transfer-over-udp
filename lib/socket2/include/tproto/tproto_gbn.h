#pragma once

#include "tproto.h"

struct tproto_gbn {
    struct {
        struct tproto tproto;
    } super;
    struct tproto_gbn_vtbl* __ops_vptr;
    size_t dw_size; // dispatch window size
};

static struct tproto_gbn_vtbl {
    int (*destroy)(struct tproto_gbn* tproto);
} __tproto_gbn_ops_vtbl = { 0 };

extern int tproto_gbn_init(struct tproto_gbn* tproto);

static inline int tproto_gbn_destroy(struct tproto_gbn* tproto) {
    return tproto->__ops_vptr->destroy(tproto);
}
