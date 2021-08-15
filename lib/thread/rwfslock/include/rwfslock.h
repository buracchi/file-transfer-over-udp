#pragma once

#include <stdbool.h>
#include <pthread.h>

#include "map/linked_list_map.h"

struct cmn_rwfslock {
	struct cmn_linked_list_map map;
	pthread_mutex_t mutex;
};

/**
 * @brief 
 * 
 * @param rwfslock 
 * @return int 
 */
extern int cmn_rwfslock_init(struct cmn_rwfslock* rwfslock);

/**
 * @brief 
 * 
 * @param rwfslock 
 * @return int 
 */
extern int cmn_rwfslock_destroy(struct cmn_rwfslock* rwfslock);

/**
 * @brief 
 * 
 * @param rwfslock 
 * @param fname 
 * @return int 
 */
extern int cmn_rwfslock_rdlock(struct cmn_rwfslock* rwfslock, const char* fname);

/**
 * @brief 
 * 
 * @param rwfslock 
 * @param fname 
 * @return int 
 */
extern int cmn_rwfslock_wrlock(struct cmn_rwfslock* rwfslock, const char* fname);

/**
 * @brief 
 * 
 * @param rwfslock 
 * @param fname 
 * @return int 
 */
extern int cmn_rwfslock_unlock(struct cmn_rwfslock* rwfslock, const char* fname);
