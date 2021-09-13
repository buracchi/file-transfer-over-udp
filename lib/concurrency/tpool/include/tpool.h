#pragma once

#include <stddef.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_tpool *cmn_tpool_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/**
 * @brief Initialize a thread pool.
 * 
 * @details The \a cmn_tpool_init() function shall initialize the thread pool
 *  referenced by @p tpool with @p tnumber threads.
 *  Attempting to initialize an already initialized thread pool results in
 *  undefined behavior.
 * 
 * @param tpool the thread pool to initialize.
 * @param tnumber Specifies the number of threads to be used.
 * @return If successful \a cmn_tpool_init() function shall return zero;
 *  otherwise, an error number shall be returned to indicate the error.
 */
extern cmn_tpool_t cmn_tpool_init(size_t tnumber);

/**
 * @brief Destroy a thread pool.
 * 
 * @details The \a cmn_tpool_destroy() function shall destroy the thread pool
 *  object referenced by @p tpool the thread pool becomes, in effect,
 *  uninitialized.
 *  A destroyed thread pool object can be reinitialized using
 *  \a cmn_tpool_init(); the results of otherwise referencing the object after
 *  it has been destroyed are undefined.
 *  It shall be safe to destroy an initialized thread pool having a non empty
 *  working queue.
 * 
 * @param tpool the thread pool to destroy.
 * @return If successful \a cmn_tpool_destroy() function shall return zero;
 *  otherwise, an error number shall be returned to indicate the error.
 */
extern int cmn_tpool_destroy(cmn_tpool_t tpool);

/**
 * @brief Add a work to the thread pool.
 * 
 * @details The \a cmn_tpool_add_work() function shall add a new work into the
 *  thread pool object referenced by @p tpool which will be assigned to a thread
 *  which will execute the routine referenced by @p work_routine passing the
 *  value referenced by @p arg as a parameter.
 *  If @p work_routine is set to @c NULL calling this function results in an
 *  undefined behaviour
 * 
 * @param tpool 
 * @param work_routine 
 * @param arg 
 * @return If successful \a cmn_tpool_add_work() function shall return zero;
 *  otherwise, an error number shall be returned to indicate the error.
 */
extern int cmn_tpool_add_work(cmn_tpool_t tpool, void *(*work_routine)(void *), void *arg);
