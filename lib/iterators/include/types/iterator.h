#pragma once

#include "iterator.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

struct cmn_iterator {
    struct cmn_iterator_vtbl *__ops_vptr;
};

static struct cmn_iterator_vtbl {
    int (*destroy)(cmn_iterator_t iterator);

    void *(*data)(cmn_iterator_t iterator);

    cmn_iterator_t (*next)(cmn_iterator_t iterator);

    cmn_iterator_t (*prev)(cmn_iterator_t iterator);

    bool (*begin)(cmn_iterator_t iterator);

    bool (*end)(cmn_iterator_t iterator);
} __cmn_iterator_ops_vtbl = {0, 0, 0, 0, 0, 0};
