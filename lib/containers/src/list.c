#include "list.h"
#include "types/list.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

extern inline int cmn_list_destroy(cmn_list_t list) {
    int ret = list->__ops_vptr->destroy(list);
    if (!ret) {
        free(list);
    }
    return ret;
}

extern inline void *cmn_list_front(cmn_list_t list) {
    return list->__ops_vptr->front(list);
}

extern inline void *cmn_list_back(cmn_list_t list) {
    return list->__ops_vptr->back(list);
}

extern inline cmn_iterator_t cmn_list_begin(cmn_list_t list) {
    return list->__ops_vptr->begin(list);
}

extern inline cmn_iterator_t cmn_list_end(cmn_list_t list) {
    return list->__ops_vptr->end(list);
}

extern inline bool cmn_list_is_empty(cmn_list_t list) {
    return list->__ops_vptr->is_empty(list);
}

extern inline size_t cmn_list_size(cmn_list_t list) {
    return list->__ops_vptr->size(list);
}

extern inline void cmn_list_clear(cmn_list_t list) {
    list->__ops_vptr->clear(list);
}

extern inline int cmn_list_insert(cmn_list_t list, cmn_iterator_t position, void *value, cmn_iterator_t *inserted) {
    return list->__ops_vptr->insert(list, position, value, inserted);
}

extern inline int cmn_list_erase(cmn_list_t list, cmn_iterator_t position, cmn_iterator_t *iterator) {
    return list->__ops_vptr->erase(list, position, iterator);
}

extern inline int cmn_list_push_front(cmn_list_t list, void *value) {
    return list->__ops_vptr->push_front(list, value);
}

extern inline int cmn_list_push_back(cmn_list_t list, void *value) {
    return list->__ops_vptr->push_back(list, value);
}

extern inline void cmn_list_pop_front(cmn_list_t list) {
    list->__ops_vptr->pop_front(list);
}

extern inline void cmn_list_pop_back(cmn_list_t list) {
    list->__ops_vptr->pop_back(list);
}

extern inline int cmn_list_resize(cmn_list_t list, size_t s, void *value) {
    return list->__ops_vptr->resize(list, s, value);
}

extern inline void cmn_list_swap(cmn_list_t list, cmn_list_t other) {
    list->__ops_vptr->swap(list, other);
}

extern inline int cmn_list_merge(cmn_list_t list, cmn_list_t other, int (*comp)(void *, void *, bool *)) {
    return list->__ops_vptr->merge(list, other, comp);
}

extern inline void cmn_list_splice(cmn_list_t list, cmn_list_t other, cmn_iterator_t position, cmn_iterator_t first,
                                   cmn_iterator_t last) {
    list->__ops_vptr->splice(list, other, position, first, last);
}

extern inline size_t cmn_list_remove(cmn_list_t list, void *value) {
    return list->__ops_vptr->remove(list, value);
}

extern inline int cmn_list_remove_if(cmn_list_t list, int (*pred)(void *, bool *), size_t *removed) {
    return list->__ops_vptr->remove_if(list, pred, removed);
}

extern inline void cmn_list_reverse(cmn_list_t list) {
    list->__ops_vptr->reverse(list);
}

extern inline int cmn_list_unique(cmn_list_t list, int (*comp)(void *, void *, bool *), size_t *removed) {
    return list->__ops_vptr->unique(list, comp, removed);
}

extern inline int cmn_list_sort(cmn_list_t list, int(*comp)(void *, void *, bool *)) {
    return list->__ops_vptr->sort(list, comp);
}
