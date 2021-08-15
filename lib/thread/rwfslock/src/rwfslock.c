#include "rwfslock.h"

#include <stdlib.h>
#include <string.h>

#include "map.h"
#include "try.h"
#include "utilities.h"

static pthread_rwlock_t* get_rwlock(struct cmn_rwfslock* this, const char* fname);
static int lexicographical_comparison(void* arg1, void* arg2, bool* result);

extern int cmn_rwfslock_init(struct cmn_rwfslock* this) {
	memset(this, 0, sizeof * this);
	try_pthread_mutex_init(&this->mutex, fail);
	try(cmn_linked_list_map_init(&(this->map)), !0, fail2);
	return 0;
fail2:
	pthread_mutex_destroy(&(this->mutex));
fail:
	return 1;
}

extern int cmn_rwfslock_destroy(struct cmn_rwfslock* this) {
	try_pthread_mutex_lock(&(this->mutex), fail);
	// TODO: destroy rwlock mutex befor destroy the map
	try(cmn_map_destroy(&(this->map.super)), !0, fail);
	try_pthread_mutex_destroy(&(this->mutex), fail);
	return 0;
fail:
	return 1;
}

extern int cmn_rwfslock_rdlock(struct cmn_rwfslock* this, const char* fname) {
	try_pthread_mutex_lock(&(this->mutex), fail);
	pthread_rwlock_t* lock = get_rwlock(this, fname);
	try_pthread_mutex_unlock(&(this->mutex), fail);
	try_pthread_rwlock_rdlock(lock, fail);
	return 0;
fail:
	return 1;
}

extern int cmn_rwfslock_wrlock(struct cmn_rwfslock* this, const char* fname) {
	try_pthread_mutex_lock(&(this->mutex), fail);
	pthread_rwlock_t* lock = get_rwlock(this, fname);
	try_pthread_mutex_unlock(&(this->mutex), fail);
	try_pthread_rwlock_wrlock(lock, fail);
	return 0;
fail:
	return 1;
}

extern int cmn_rwfslock_unlock(struct cmn_rwfslock* this, const char* fname) {
	try_pthread_mutex_lock(&(this->mutex), fail);
	pthread_rwlock_t* lock = get_rwlock(this, fname);
	try_pthread_mutex_unlock(&(this->mutex), fail);
	try_pthread_rwlock_unlock(lock, fail);
	return 0;
fail:
	return 1;
}

static pthread_rwlock_t* get_rwlock(struct cmn_rwfslock* this, const char* fname) {
	pthread_rwlock_t* lock;
	if (cmn_map_at2(&(this->map.super), (void*)fname, lexicographical_comparison, (void**)&lock)) {
		lock = malloc(sizeof *lock);
		try_pthread_rwlock_init(lock, fail);
		cmn_map_insert(&(this->map.super), (void*)fname, lock);
	}
	return lock;
fail:
	return NULL;
}

static int lexicographical_comparison(void* arg1, void* arg2, bool* result) {
	char* _s1 = (char*)arg1;
	char* _s2 = (char*)arg2;
	while(*_s1 && *_s2 && *(_s1++) == *(_s2++));
    *result = *_s1 == *_s2;
	return 0;
}
