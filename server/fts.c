#include "fts.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "socket2.h"
#include "tpool.h"
#include "try.h"
#include "utilities.h"

#define BACKLOG 4096

struct dispatcher_thread_arg {
	socket2_t listening_socket;
	tpool_t thread_pool;
};

static void fts_close(evutil_socket_t signal, short events, void* arg);
static void dispatch_request(evutil_socket_t fd, short events, void* arg);
static void* handle_request(void* arg);
static struct event_base* get_thread_event_base();

extern int fts_start(int port, char* pathname) {
	struct event_base* base;
	struct event* socket_event;
	struct event* signal_event;
	struct dispatcher_thread_arg* dta;
	tpool_t tpool;
	socket2_t socket;
	try(evthread_use_pthreads(), -1, fail);
	try(tpool = tpool_init(get_nprocs()), NULL, fail);
	try(socket = socket2_init(TCP, IPV4), NULL, fail);
	try(socket2_ipv4_setaddr(socket, "127.0.0.1", port), 1, fail);
	try(socket2_set_blocking(socket, false), 1, fail);
	try(socket2_listen(socket, BACKLOG), 1, fail);
	try(base = get_thread_event_base(), NULL, fail);
	try(dta = malloc(sizeof * dta), NULL, fail);
	dta->listening_socket = socket;
	dta->thread_pool = tpool;
	try(socket_event = event_new(base, socket2_get_fd(socket), EV_READ | EV_PERSIST, dispatch_request, dta), NULL, fail);
	try(signal_event = evsignal_new(base, SIGINT, fts_close, (void*)base), NULL, fail);
	try(event_add(socket_event, NULL), -1, fail);
	try(event_add(signal_event, NULL), -1, fail);
	try((printf("Server started.\n") < 0), true, fail);
	try(event_base_dispatch(base), -1, fail);
	try(tpool_wait(tpool), 1, fail);
	event_free(socket_event);
	event_free(signal_event);
	event_base_free(base);
	libevent_global_shutdown();
	free(dta);
	try(socket2_close(socket), 1, fail);
	socket2_destroy(socket);
	try(tpool_destroy(tpool), 1, fail);
	return 0;
fail:
	return 1;
}

static void fts_close(evutil_socket_t signal, short events, void* user_data) {
	struct event_base* base = user_data;
	struct timeval delay = { 2, 0 };
	printf("\nShutting down the server...\n");
	event_base_loopexit(base, &delay);
}

static void dispatch_request(evutil_socket_t fd, short events, void* arg) {
	struct dispatcher_thread_arg* dta = arg;
	socket2_t accepted = socket2_accept(dta->listening_socket);
	tpool_add_work(dta->thread_pool, handle_request, accepted);
}

static void* handle_request(void* arg) {
	socket2_t socket = arg;
	char* buff;
	socket2_recv(socket, &buff);
	sleep(1);
	printf("%s\n", buff);
	free(buff);
	socket2_close(socket);
	socket2_destroy(socket);
}

static struct event_base* get_thread_event_base() {
	return event_base_new();
}
