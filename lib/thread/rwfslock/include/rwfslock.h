#pragma once

#include <stdbool.h>
#include <pthread.h>

struct dictionary { // thread unsafe
};
static int dictionary_init(struct dictionary* dictionary, bool(*cmp)(void*, void*));
static int dictionary_free(struct dictionary* dictionary);
static int dictionary_get(struct dictionary* dictionary, const void* key, void** value);
static int dictionary_put(struct dictionary* dictionary, const void* key, const void* value);

struct rwfslock {	// thread safe
	struct dictionary dictionary;
	pthread_mutex_t mutex;
	struct rwfslock_vtbl* __ops_vptr;
};

static struct rwfslock_vtbl {
	int (*destroy)(struct rwfslock*);
	int (*rdlock)(struct rwfslock* rwfslock, const char* fname);
	int (*wrlock)(struct rwfslock* rwfslock, const char* fname);
	int (*unlock)(struct rwfslock* rwfslock, const char* fname);
} __rwfslock_ops_vtbl = { 0 };


extern int rwfslock_init(struct rwfslock* rwfslock);

static int rwfslock_destroy(struct rwfslock* rwfslock) {
	return rwfslock->__ops_vptr->destroy(rwfslock);
}

static int rwfslock_rdlock(struct rwfslock* rwfslock, const char* fname) {
	return rwfslock->__ops_vptr->rdlock(rwfslock, fname);
}

static int rwfslock_wrlock(struct rwfslock* rwfslock, const char* fname) {
	return rwfslock->__ops_vptr->wrlock(rwfslock, fname);
}

static int rwfslock_unlock(struct rwfslock* rwfslock, const char* fname) {
	return rwfslock->__ops_vptr->unlock(rwfslock, fname);
}
