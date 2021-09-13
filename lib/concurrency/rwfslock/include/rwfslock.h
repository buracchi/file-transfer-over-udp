#pragma once

typedef struct cmn_rwfslock *cmn_rwfslock_t;

/**
 * @brief 
 * 
 * @return cmn_rwfslock_t 
 */
extern cmn_rwfslock_t cmn_rwfslock_init();

/**
 * @brief 
 * 
 * @param rwfslock 
 * @return int 
 */
extern int cmn_rwfslock_destroy(cmn_rwfslock_t rwfslock);

/**
 * @brief 
 * 
 * @param rwfslock 
 * @param fname 
 * @return int 
 */
extern int cmn_rwfslock_rdlock(cmn_rwfslock_t rwfslock, const char *fname);

/**
 * @brief 
 * 
 * @param rwfslock 
 * @param fname 
 * @return int 
 */
extern int cmn_rwfslock_wrlock(cmn_rwfslock_t rwfslock, const char *fname);

/**
 * @brief 
 * 
 * @param rwfslock 
 * @param fname 
 * @return int 
 */
extern int cmn_rwfslock_unlock(cmn_rwfslock_t rwfslock, const char *fname);
