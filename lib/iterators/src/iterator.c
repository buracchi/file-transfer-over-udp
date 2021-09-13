#include "iterator.h"
#include "types/iterator.h"

#include <stdlib.h>

extern inline int cmn_iterator_destroy(cmn_iterator_t iterator) {
    int ret = iterator->__ops_vptr->destroy(iterator);
    if (!ret) {
        free(iterator);
    }
    return ret;
}

extern inline void *cmn_iterator_data(cmn_iterator_t iterator) {
    return iterator->__ops_vptr->data(iterator);
}

extern inline cmn_iterator_t cmn_iterator_next(cmn_iterator_t iterator) {
    return iterator->__ops_vptr->next(iterator);
}

extern inline cmn_iterator_t cmn_iterator_prev(cmn_iterator_t iterator) {
    return iterator->__ops_vptr->prev(iterator);
}

extern inline bool cmn_iterator_begin(cmn_iterator_t iterator) {
    return iterator->__ops_vptr->begin(iterator);
}

extern inline bool cmn_iterator_end(cmn_iterator_t iterator) {
    return iterator->__ops_vptr->end(iterator);
}

extern cmn_iterator_t cmn_iterator_advance(cmn_iterator_t iterator, int n) {
    for (int i = n; i != 0; n ? i++ : i--) {
        n ? iterator->__ops_vptr->next(iterator) : iterator->__ops_vptr->prev(iterator);
    }
    return iterator;
}

extern size_t cmn_iterator_distance(cmn_iterator_t first, cmn_iterator_t last) {
    size_t dist = 0;
    while (first != last) {
        first->__ops_vptr->next(first);
        dist++;
    }
    for (size_t i = 0; i < dist; i++) {
        first->__ops_vptr->prev(first);        //Not very smart duh
    }
    return dist;
}
