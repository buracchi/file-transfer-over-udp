#pragma once

#include <stdbool.h>

#include "iterator.h"

typedef struct cmn_map {
    struct cmn_map_vtbl *__ops_vptr;

    int (*comp)(void *a, void *b, bool *result);
} *cmn_map_t;

static struct cmn_map_vtbl {
    // Member functions
    int (*destroy)(cmn_map_t map);

    // Element access
    int (*at)(cmn_map_t map, void *key, void **value);

    // Iterators
    cmn_iterator_t (*begin)(cmn_map_t map);

    cmn_iterator_t (*end)(cmn_map_t map);
    // Capacity
    bool (*is_empty)(cmn_map_t map);

    size_t (*size)(cmn_map_t map);

    // Modifiers
    void (*set_key_comparer)(cmn_map_t map, int (*comp)(void *a, void *b, bool *result));

    void (*clear)(cmn_map_t map);

    int (*insert)(cmn_map_t map, void *key, void *value, cmn_iterator_t *iterator);

    int (*insert_or_assign)(cmn_map_t map, void *key, void *value, cmn_iterator_t *iterator);

    int (*erase)(cmn_map_t map, cmn_iterator_t position, cmn_iterator_t *iterator);

    void (*swap)(cmn_map_t map, cmn_map_t other);

    // <T> (_*extract)(cmn_map_t map, ...)
    // Lookup
    int (*count)(cmn_map_t map, void *key, size_t *count);

    int (*find)(cmn_map_t map, void *key, cmn_iterator_t *iterator);

    bool (*contains)(cmn_map_t map, void *key);
} __cmn_map_ops_vtbl = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
