#include "map.h"
#include "map/linked_list_map.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "types/map.h"
#include "types/map/linked_list_map.h"
#include "utilities.h"
#include "try.h"

#define KEY first
#define VALUE second

static struct cmn_map_vtbl *get_map_vtbl();

static int _destroy(cmn_map_t map);

static int _at(cmn_map_t map, void *key, void **value);

static cmn_iterator_t _begin(cmn_map_t map);

static cmn_iterator_t _end(cmn_map_t map);

static bool _is_empty(cmn_map_t map);

static size_t _size(cmn_map_t map);

static void _set_key_comparer(cmn_map_t map, int (*comp)(void *a, void *b, bool *result));

static void _clear(cmn_map_t map);

static int _insert(cmn_map_t map, void *key, void *value, cmn_iterator_t *iterator);

static int _insert_or_assign(cmn_map_t map, void *key, void *value, cmn_iterator_t *iterator);

static int _erase(cmn_map_t map, cmn_iterator_t position, cmn_iterator_t *iterator);

static void _swap(cmn_map_t map, cmn_map_t other);

static int _count(cmn_map_t map, void *key, size_t *count);

static int _find(cmn_map_t map, void *key, cmn_iterator_t *iterator);

static bool _contains(cmn_map_t map, void *key);

static inline int dflt_cmp(void *arg1, void *arg2, bool *result) {
    *result = (arg1 == arg2);
    return 0;
}

extern cmn_linked_list_map_t cmn_linked_list_map_init() {
    cmn_linked_list_map_t map;
    try(map = malloc(sizeof *map), NULL, fail);
    try(cmn_linked_list_map_ctor(map), 1, fail2);
    return map;
fail2:
    free(map);
fail:
    return NULL;
}

extern int cmn_linked_list_map_ctor(cmn_linked_list_map_t this) {
    memset(this, 0, sizeof *this);
    this->super.__ops_vptr = get_map_vtbl();
    this->super.comp = dflt_cmp;
    try(this->list = cmn_linked_list_init(), NULL, fail);
    return 0;
fail:
    return 1;
}

static struct cmn_map_vtbl *get_map_vtbl() {
    struct cmn_map_vtbl vtbl_zero = {0};
    if (!memcmp(&vtbl_zero, &__cmn_map_ops_vtbl, sizeof vtbl_zero)) {
        __cmn_map_ops_vtbl.destroy = _destroy;
        __cmn_map_ops_vtbl.at = _at;
        __cmn_map_ops_vtbl.begin = _begin;
        __cmn_map_ops_vtbl.end = _end;
        __cmn_map_ops_vtbl.is_empty = _is_empty;
        __cmn_map_ops_vtbl.size = _size;
        __cmn_map_ops_vtbl.set_key_comparer = _set_key_comparer;
        __cmn_map_ops_vtbl.clear = _clear;
        __cmn_map_ops_vtbl.insert = _insert;
        __cmn_map_ops_vtbl.insert_or_assign = _insert_or_assign;
        __cmn_map_ops_vtbl.erase = _erase;
        __cmn_map_ops_vtbl.swap = _swap;
        __cmn_map_ops_vtbl.count = _count;
        __cmn_map_ops_vtbl.find = _find;
        __cmn_map_ops_vtbl.contains = _contains;
    }
    return &__cmn_map_ops_vtbl;
}

static int _destroy(cmn_map_t map) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    _clear(map);
    return cmn_list_destroy((cmn_list_t) this->list);
}

static int _at(cmn_map_t map, void *key, void **value) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    cmn_iterator_t iterator;
    iterator = cmn_list_begin((cmn_list_t) this->list);
    while (cmn_iterator_end(iterator) == false) {
        cmn_pair_t kv_pair;
        bool found;
        kv_pair = (cmn_pair_t) cmn_iterator_data(iterator);
        try(this->super.comp(kv_pair->KEY, key, &found), !0, fail);
        if (found) {
            *value = kv_pair->VALUE;
            cmn_iterator_destroy(iterator);
            return 0;
        }
        iterator = cmn_iterator_next(iterator);
    }
    cmn_iterator_destroy(iterator);
    errno = ERANGE;
fail:
    return 1;
}

static cmn_iterator_t _begin(cmn_map_t map) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    return cmn_list_begin((cmn_list_t) this->list);
}

static cmn_iterator_t _end(cmn_map_t map) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    return cmn_list_end((cmn_list_t) this->list);
}

static bool _is_empty(cmn_map_t map) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    return cmn_list_is_empty((cmn_list_t) this->list);
}

static size_t _size(cmn_map_t map) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    return cmn_list_size((cmn_list_t) this->list);
}

static void _set_key_comparer(cmn_map_t map, int (*comp)(void *, void *, bool *)) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    this->super.comp = comp;
}

static void _clear(cmn_map_t map) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    cmn_iterator_t iterator;
    iterator = cmn_list_begin((cmn_list_t) this->list);
    while (cmn_iterator_end(iterator) == false) {
        cmn_pair_t kv_pair = (cmn_pair_t) cmn_iterator_data(iterator);
        free(kv_pair);
        iterator = cmn_iterator_next(iterator);
    }
    cmn_iterator_destroy(iterator);
    cmn_list_clear((cmn_list_t) this->list);
}

static int _insert(cmn_map_t map, void *key, void *value, cmn_iterator_t *inserted) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    cmn_pair_t kv_pair = malloc(sizeof *kv_pair);
    if (kv_pair) {
        kv_pair->KEY = key;
        kv_pair->VALUE = value;
        cmn_iterator_t iterator = cmn_list_begin((cmn_list_t) this->list);
        cmn_list_insert((cmn_list_t) this->list, iterator, kv_pair, inserted);
        cmn_iterator_destroy(iterator);
    }
    return 0;
}

static int _insert_or_assign(cmn_map_t map, void *key, void *value, cmn_iterator_t *inserted) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    cmn_pair_t kv_pair;
    if (_at(map, key, (void **) &kv_pair)) {
        return _insert(map, key, value, inserted);
    }
    kv_pair->VALUE = value;
    if (inserted) {
        cmn_iterator_t iterator = cmn_list_begin((cmn_list_t) this->list);
        while (((cmn_pair_t) cmn_iterator_data(iterator))->KEY != key) {
            iterator = cmn_iterator_next(iterator);
        }
        *inserted = iterator;
    }
    return 0;
}

static int _erase(cmn_map_t map, cmn_iterator_t position, cmn_iterator_t *iterator) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    cmn_pair_t kv_pair = (cmn_pair_t) cmn_iterator_data(position);
    free(kv_pair);
    cmn_list_erase((cmn_list_t) this->list, position, iterator);
    return 0;
}

static void _swap(cmn_map_t map, cmn_map_t other) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    cmn_linked_list_map_t _other = (cmn_linked_list_map_t) other;
    cmn_list_swap((cmn_list_t) this->list, (cmn_list_t) _other->list);
}

static int _count(cmn_map_t map, void *key, size_t *count) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    cmn_iterator_t iterator;
    *count = 0;
    iterator = cmn_list_begin((cmn_list_t) this->list);
    while (cmn_iterator_end(iterator) == false) {
        cmn_pair_t kv_pair = (cmn_pair_t) cmn_iterator_data(iterator);
        bool found;
        try(this->super.comp(kv_pair->KEY, key, &found), !0, fail);
        if (found) {
            *count = 1;
            break;
        }
        iterator = cmn_iterator_next(iterator);
    }
    cmn_iterator_destroy(iterator);
    return 0;
fail:
    cmn_iterator_destroy(iterator);
    return 1;
}

static int _find(cmn_map_t map, void *key, cmn_iterator_t *iterator) {
    cmn_linked_list_map_t this = (cmn_linked_list_map_t) map;
    cmn_iterator_t i;
    i = cmn_list_begin((cmn_list_t) this->list);
    while (cmn_iterator_end(i) == false) {
        cmn_pair_t kv_pair = (cmn_pair_t) cmn_iterator_data(i);
        bool found;
        try(this->super.comp(kv_pair->KEY, key, &found), !0, fail);
        if (found) {
            *iterator = i;
            return 0;
        }
        i = cmn_iterator_next(i);
    }
fail:
    cmn_iterator_destroy(i);
    return 1;
}

static bool _contains(cmn_map_t map, void *key) {
    return !(_at(map, key, NULL) && errno == ERANGE);
}
