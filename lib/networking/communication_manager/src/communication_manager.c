#include "communication_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "request_handler.h"
#include "try.h"
#include "utilities.h"

#ifdef __unix__
	#include <sys/sysinfo.h>
	#include <unistd.h>
#endif

#define BACKLOG 4096

#ifdef WIN32
	#define evthread_use_threads evthread_use_windows_threads
#elif __unix__
	#define evthread_use_threads evthread_use_pthreads
#endif

static void stop(evutil_socket_t signal, short events, void* arg);
static void dispatch_request(evutil_socket_t fd, short events, void* arg);
static void* start_worker(void* arg);

static struct event_base* event_base;

extern int cmn_communication_manager_init(struct cmn_communication_manager* this, size_t thread_number) {
	if (thread_number) {
		try(evthread_use_threads(), -1, fail);
		try(cmn_tpool_init(&(this->thread_pool), thread_number), !0, fail);
	} else {
		goto fail;	// TODO: handle mono thread case
	}
	return 0;
fail:
	return 1;
}

extern int cmn_communication_manager_start(struct cmn_communication_manager* this, struct cmn_nproto_service* nproto_serivce, struct cmn_tproto_service* tproto_serivce, const char* url, struct cmn_request_handler* request_handler) {
	struct event* socket_event;
	struct event* signal_event;
	try(cmn_socket2_init(&(this->socket), nproto_serivce, tproto_serivce), !0, fail);
	try(cmn_socket2_set_blocking(&(this->socket), false), 1, fail);
	try(cmn_socket2_listen(&(this->socket), url, BACKLOG), 1, fail);
	this->handler = request_handler;
	try(event_base = event_base_new(), NULL, fail);
	try(socket_event = event_new(event_base, this->socket.fd, EV_READ | EV_PERSIST, dispatch_request, this), NULL, fail);
	try(signal_event = evsignal_new(event_base, SIGINT, stop, (void*)event_base), NULL, fail);
	try(event_add(socket_event, NULL), -1, fail);
	try(event_add(signal_event, NULL), -1, fail);
	try((printf("Server started.\n") < 0), true, fail);
	try(event_base_dispatch(event_base), -1, fail);
	event_free(socket_event);
	event_free(signal_event);
	event_base_free(event_base);
	libevent_global_shutdown();
	try(cmn_socket2_destroy(&(this->socket)), 1, fail);
	return 0;
fail:
	return 1;
}

extern int cmn_communication_manager_stop(struct cmn_communication_manager* this) {
	stop(0, 0, (void*)event_base);
}

extern int cmn_communication_manager_destroy(struct cmn_communication_manager* this) {
	try(cmn_tpool_destroy(&(this->thread_pool)), 1, fail);
	return 0;
fail:
	return 1;
}

static void dispatch_request(evutil_socket_t fd, short events, void* arg) {
	struct cmn_communication_manager* this = (struct cmn_communication_manager*)arg;
	cmn_tpool_add_work(&(this->thread_pool), start_worker, arg);
}
static void* start_worker(void* arg) {
	struct cmn_communication_manager* this = (struct cmn_communication_manager*)arg;
	struct cmn_socket2* accepted;
	accepted = malloc(sizeof *accepted);
	if (!cmn_socket2_accept(&(this->socket), accepted)) {
		cmn_request_handler_handle_request(this->handler, accepted);
	}
}

static void stop(evutil_socket_t signal, short events, void* user_data) {
	struct event_base* base = user_data;
	struct timeval delay = { 1, 0 };
	printf("\nShutting down...\n");
	event_base_loopexit(base, &delay);
}
