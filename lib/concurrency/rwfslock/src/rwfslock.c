#include "rwfslock.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include "map/linked_list_map.h"
#include "try.h"
#include "utilities.h"

struct cmn_rwfslock {
    cmn_map_t map;
    pthread_mutex_t mutex;
};

static pthread_rwlock_t *get_rwlock(cmn_rwfslock_t this, const char *fname);

static int lexicographical_comparison(void *arg1, void *arg2, bool *result) {
    const char *_s1 = (const char *) arg1;
    const char *_s2 = (const char *) arg2;
    while (*_s1 && *_s2 && *(_s1++) == *(_s2++)) {}
    *result = *_s1 == *_s2;
    return 0;
}

extern cmn_rwfslock_t cmn_rwfslock_init() {
    cmn_rwfslock_t this;
    try(this = malloc(sizeof *this), NULL, fail);
    memset(this, 0, sizeof *this);
    try(this->map = (cmn_map_t) cmn_linked_list_map_init(), NULL, fail);
    cmn_map_set_key_comparer(this->map, lexicographical_comparison);
    try_pthread_mutex_init(&(this->mutex), fail2);
    return this;
fail2:
    cmn_map_destroy(this->map);
fail:
    return NULL;
}

extern int cmn_rwfslock_destroy(cmn_rwfslock_t this) {
    struct cmn_iterator *iterator;
    try_pthread_mutex_lock(&(this->mutex), fail);
    iterator = cmn_map_begin(this->map);
    while (!cmn_iterator_end(iterator)) {
        struct cmn_pair *kv_pair;
        kv_pair = cmn_iterator_data(iterator);
        try_pthread_rwlock_destroy(kv_pair->second, fail);
        free(kv_pair->second);
        iterator = cmn_iterator_next(iterator);
    }
    cmn_iterator_destroy(iterator);
    try(cmn_map_destroy(this->map), !0, fail);
    try_pthread_mutex_destroy(&(this->mutex), fail);
    free(this);
    return 0;
fail:
    return 1;
}

extern int cmn_rwfslock_rdlock(cmn_rwfslock_t this, const char *fname) {
    try_pthread_mutex_lock(&(this->mutex), fail);
    pthread_rwlock_t *lock = get_rwlock(this, fname);
    try_pthread_mutex_unlock(&(this->mutex), fail);
    try_pthread_rwlock_rdlock(lock, fail);
    return 0;
fail:
    return 1;
}

extern int cmn_rwfslock_wrlock(cmn_rwfslock_t this, const char *fname) {
    try_pthread_mutex_lock(&(this->mutex), fail);
    pthread_rwlock_t *lock = get_rwlock(this, fname);
    try_pthread_mutex_unlock(&(this->mutex), fail);
    try_pthread_rwlock_wrlock(lock, fail);
    return 0;
fail:
    return 1;
}

extern int cmn_rwfslock_unlock(cmn_rwfslock_t this, const char *fname) {
    try_pthread_mutex_lock(&(this->mutex), fail);
    pthread_rwlock_t *lock = get_rwlock(this, fname);
    try_pthread_mutex_unlock(&(this->mutex), fail);
    try_pthread_rwlock_unlock(lock, fail);
    return 0;
fail:
    return 1;
}

static pthread_rwlock_t *get_rwlock(cmn_rwfslock_t this, const char *fname) {
    pthread_rwlock_t *lock;
    if (cmn_map_at(this->map, (void *) fname, (void **) &lock)) {
        lock = malloc(sizeof *lock);
        try_pthread_rwlock_init(lock, fail);
        cmn_map_insert(this->map, (void *) fname, lock, NULL);
    }
    return lock;
fail:
    return NULL;
}
