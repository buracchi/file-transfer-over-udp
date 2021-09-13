#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "iterator.h"

typedef struct cmn_list {
    struct cmn_list_vtbl *__ops_vptr;
} *cmn_list_t;

static struct cmn_list_vtbl {
    // Member functions
    int (*destroy)(cmn_list_t list);

    // Element access
    void *(*front)(cmn_list_t list);

    void *(*back)(cmn_list_t list);

    // Iterators
    cmn_iterator_t (*begin)(cmn_list_t list);

    cmn_iterator_t (*end)(cmn_list_t list);
    // Capacity
    bool (*is_empty)(cmn_list_t list);

    size_t (*size)(cmn_list_t list);

    // Modifiers
    void (*clear)(cmn_list_t list);

    int (*insert)(cmn_list_t list, cmn_iterator_t position, void *value, cmn_iterator_t *inserted);

    int (*erase)(cmn_list_t list, cmn_iterator_t position, cmn_iterator_t *iterator);

    int (*push_front)(cmn_list_t list, void *value);

    int (*push_back)(cmn_list_t list, void *value);

    void (*pop_front)(cmn_list_t list);

    void (*pop_back)(cmn_list_t list);

    int (*resize)(cmn_list_t list, size_t s, void *value);

    void (*swap)(cmn_list_t list, cmn_list_t other);

    // Operations
    int (*merge)(cmn_list_t list, cmn_list_t other, int(*comp)(void *a, void *b, bool *result));

    void (*splice)(cmn_list_t list, cmn_list_t other, cmn_iterator_t pos, cmn_iterator_t first, cmn_iterator_t last);

    size_t (*remove)(cmn_list_t list, void *value);

    int (*remove_if)(cmn_list_t list, int(*p)(void *a, bool *result), size_t *removed);

    void (*reverse)(cmn_list_t list);

    int (*unique)(cmn_list_t list, int(*comp)(void *a, void *b, bool *result), size_t *removed);

    int (*sort)(cmn_list_t list, int(*comp)(void *a, void *b, bool *result));
} __cmn_list_ops_vtbl = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
