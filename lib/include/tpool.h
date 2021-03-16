#pragma once

#include <stddef.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef void* tpool_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

extern tpool_t tpool_init(size_t tnumber);

extern int tpool_destroy(const tpool_t handle);

extern int tpool_add_work(const tpool_t handle, void* (*work_routine) (void*), void* arg);

extern int tpool_wait(const tpool_t handle);
