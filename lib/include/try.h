#pragma once

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define GET_MACRO_3(ARG1, ARG2, ARG3, NAME, ...) NAME
#define try(...) GET_MACRO_3(__VA_ARGS__, try_routine, try_default)(__VA_ARGS__)

#define try_default(predicate, error_value) \
	do { \
		errno = 0; \
		if ((predicate) == (error_value)) { \
			fprintf( \
				stderr, \
				"%m was generated in %s on line %d, in %s\n", \
				__FILE__, \
				__LINE__, \
				__FUNCTION__ \
			); \
			exit(EXIT_FAILURE); \
		} \
	} while (0)

#define try_routine(predicate, error_value, routine) \
	do { \
		errno = 0; \
		if ((predicate) == (error_value)) { \
			goto routine; \
		} \
	} while (0)

#define try_pthread(foo, routine) \
	do { \
		int ret; \
		errno = 0; \
		while ((ret = foo) && errno == EINTR); \
		if (ret) { \
			goto routine; \
		} \
	} while (0)

#define try_pthread_create(thread, attr, start_routine, arg, routine) \
	try_pthread(pthread_create(thread, attr, start_routine, arg), routine)

#define try_pthread_detach(thread, routine) \
	try_pthread(pthread_detach(thread), routine)

#define try_pthread_cond_wait(cond, mutex, routine) \
	try_pthread(pthread_cond_wait(cond, mutex), routine)

#define try_pthread_mutex_init(mutex, routine) \
	try_pthread(pthread_mutex_init(mutex, NULL), routine)

#define try_pthread_mutex_destroy(mutex, routine) \
	try_pthread(pthread_mutex_destroy(mutex), routine)

#define try_pthread_mutex_unlock(mutex, routine) \
	try_pthread(pthread_mutex_unlock(mutex), routine)

#define try_pthread_mutex_lock(mutex, routine) \
	try_pthread(pthread_mutex_lock(mutex), routine)

#define try_pthread_rwlock_init(rwlock, routine) \
	try_pthread(pthread_rwlock_init(rwlock, NULL), routine)

#define try_pthread_rwlock_destroy(rwlock, routine) \
	try_pthread(pthread_rwlock_destroy(rwlock), routine)

#define try_pthread_rwlock_rdlock(rwlock, routine) \
	try_pthread(pthread_rwlock_rdlock(rwlock), routine)

#define try_pthread_rwlock_wrlock(rwlock, routine) \
	try_pthread(pthread_rwlock_wrlock(rwlock), routine)

#define try_pthread_rwlock_unlock(rwlock, routine) \
	try_pthread(pthread_rwlock_unlock(rwlock), routine)

#define try_pthread_cond_init(cond, routine) \
	try_pthread(pthread_cond_init(cond, NULL), routine)

#define try_pthread_cond_destroy(cond, routine) \
	try_pthread(pthread_cond_destroy(cond), routine)

#define try_pthread_cond_broadcast(cond, routine) \
	try_pthread(pthread_cond_broadcast(cond), routine)

#define try_pthread_cond_signal(cond, routine) \
	try_pthread(pthread_cond_signal(cond), routine)

#define try_pthread_cond_wait(cond, mutex, routine) \
	try_pthread(pthread_cond_wait(cond, mutex), routine)
