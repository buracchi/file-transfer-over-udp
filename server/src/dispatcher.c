#include "dispatcher.h"

#include <event2/event.h>
#include <event2/thread.h>

#include <buracchi/common/logger/logger.h>
#include <buracchi/common/utilities/try.h>

static void stop_on_signal_handler(int signal, short events, void *arg);

extern int dispatcher_init(dispatcher_t *dispatcher, int thread_count) {
	enum event_method_feature evmthd_feature;
#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
	try(evthread_use_windows_threads(), -1, evthread_setup_fail);
#endif
#ifdef EVTHREAD_USE_PTHREADS_IMPLEMENTED
	try(evthread_use_pthreads(), -1, evthread_setup_fail);
#endif
	cmn_logger_log_info("Dispatcher: Using Libevent version: %s", event_get_version());
	try(dispatcher->event_base = event_base_new(), NULL, event_base_new_fail);
	evmthd_feature = event_base_get_features(dispatcher->event_base);
	cmn_logger_log_info("Dispatcher: Using Libevent with backend method %s. %s %s %s",
	                    event_base_get_method(dispatcher->event_base),
	                    (evmthd_feature & EV_FEATURE_ET) ? "Edge-triggered events are supported." : "",
	                    (evmthd_feature & EV_FEATURE_O1) ? "O(1) event notification is supported." : "",
	                    (evmthd_feature & EV_FEATURE_FDS) ? "All FD types are supported." : "");
	// TODO: Initialize the thread pool
	return 0;
event_base_new_fail:
	cmn_logger_log_error("Dispatcher: Couldn't initialize an event base.");
	return -1;
evthread_setup_fail:
	cmn_logger_log_error("Dispatcher: Couldn't setup Libevent threads handling support.");
	return -1;
}

extern int dispatcher_dispatch(dispatcher_t *dispatcher) {
	return event_base_dispatch(dispatcher->event_base);
}

extern int dispatcher_stop(dispatcher_t *dispatcher) {
	return event_base_loopexit(dispatcher->event_base, NULL);
}

extern dispatcher_event_t dispatcher_signal_event_add(dispatcher_t *dispatcher, int signal, event_callback_fn callback, void *arg) {
	dispatcher_event_t signal_event;
	try(signal_event = evsignal_new(dispatcher->event_base, signal, callback, arg), NULL, event_new_fail);
	try(evsignal_add(signal_event, NULL), -1, evsignal_add_fail);
	return signal_event;
evsignal_add_fail:
	cmn_logger_log_error("Dispatcher: Couldn't set %s event status to pending.", strsignal(signal));
	return NULL;
event_new_fail:
	cmn_logger_log_error("Dispatcher: Couldn't create the %s event.", strsignal(signal));
	return NULL;
}

extern dispatcher_event_t dispatcher_stop_on_signal_event_add(dispatcher_t *dispatcher, int signal) {
	return dispatcher_signal_event_add(dispatcher, signal, stop_on_signal_handler, dispatcher);
}

extern int dispatcher_event_remove(dispatcher_event_t event) {
	int ret;
	try(ret = event_del(event), -1, fail);
	event_free(event);
	return ret;
fail:
	return ret;
}

extern void dispatcher_destroy(dispatcher_t *dispatcher) {
	// TODO: Destroy thread_pool
	event_base_free(dispatcher->event_base);
	libevent_global_shutdown();
}

static void stop_on_signal_handler(int signal, short events, void *arg) {
	cmn_logger_log_info("Dispatcher: Received %s, shutting down...", strsignal(signal));
	dispatcher_stop((dispatcher_t *)arg);
}
