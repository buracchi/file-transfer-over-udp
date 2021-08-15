#pragma once

#include <stdbool.h>
#include <pthread.h>

#include "map/linked_list_map.h"

struct rwfslock {
	struct cmn_linked_list_map map;
	pthread_mutex_t mutex;
};

extern int rwfslock_init(struct rwfslock* rwfslock);

extern int rwfslock_destroy(struct rwfslock* rwfslock);

extern int rwfslock_rdlock(struct rwfslock* rwfslock, const char* fname);

extern int rwfslock_wrlock(struct rwfslock* rwfslock, const char* fname);

extern int rwfslock_unlock(struct rwfslock* rwfslock, const char* fname);
