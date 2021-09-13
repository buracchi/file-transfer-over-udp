#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct cmn_queue {
    struct cmn_queue_vtbl *__ops_vptr;
} *cmn_queue_t;

static struct cmn_queue_vtbl {
    int (*destroy)(cmn_queue_t queue);

    bool (*is_empty)(cmn_queue_t queue);

    int (*enqueue)(cmn_queue_t queue, void *item);

    void *(*dequeue)(cmn_queue_t queue);
} __cmn_queue_ops_vtbl = {0, 0, 0, 0};
