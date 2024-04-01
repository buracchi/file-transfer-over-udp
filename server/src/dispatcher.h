#pragma once

#include <event2/event.h>

typedef struct event* dispatcher_event_t;

typedef struct dispatcher {
	struct event_base *event_base;
	// TODO: thread_pool_t thread_pool;
} dispatcher_t;

extern int dispatcher_init(dispatcher_t *dispatcher, int thread_count);

extern int dispatcher_dispatch(dispatcher_t *dispatcher);

extern int dispatcher_stop(dispatcher_t *dispatcher);

extern dispatcher_event_t dispatcher_signal_event_add(dispatcher_t *dispatcher, int signal, event_callback_fn callback, void* arg);

extern dispatcher_event_t dispatcher_stop_on_signal_event_add(dispatcher_t *dispatcher, int signal);

extern int dispatcher_event_remove(dispatcher_event_t event);

extern void dispatcher_destroy(dispatcher_t *dispatcher);
