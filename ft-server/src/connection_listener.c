#include "connection_listener.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "ft_handler.h"
#include "socket2.h"
#include "nproto/nproto_ipv4.h"
#include "tproto/tproto_tcp.h"
#include "tpool.h"
#include "try.h"
#include "utilities.h"

#ifdef __unix__
#include <sys/sysinfo.h>
#include <unistd.h>
#endif

#define BACKLOG 4096

#ifdef WIN32
#define evthread_use_threads evthread_use_windows_threads
#define nprocs() 2
#elif __unix__
#define nprocs() get_nprocs()
#define evthread_use_threads evthread_use_pthreads
#endif

static void connection_listener_close(evutil_socket_t signal, short events, void* arg);
static void dispatch_request(evutil_socket_t fd, short events, void* arg);
static void* start_worker(void* arg);

static ft_handler_t handler;
static tpool_t thread_pool;
static struct event_base* event_base;

extern int connection_listener_start(int port, ft_handler_t ft_handler) {
	struct event* socket_event;
	struct event* signal_event;
	struct nproto_ipv4 ipv4;
	struct tproto_tcp tcp;
	struct socket2* socket;
	char* url;
	handler = ft_handler;
	try(asprintf(&url, "127.0.0.1:%d", port), -1, fail);
	try(thread_pool = tpool_init(nprocs()), NULL, fail);
	try(evthread_use_threads(), -1, fail);
	try(event_base = event_base_new(), NULL, fail);
	nproto_ipv4_init(&ipv4);
	tproto_tcp_init(&tcp);
	try(socket = new(socket2, &tcp.super.tproto, &ipv4.super.nproto, true), NULL, fail);
	try(socket2_set_blocking(socket, false), 1, fail);
	try(socket2_listen(socket, url, BACKLOG), 1, fail);
	try(socket_event = event_new(event_base, socket2_get_fd(socket), EV_READ | EV_PERSIST, dispatch_request, socket), NULL, fail);
	try(signal_event = evsignal_new(event_base, SIGINT, connection_listener_close, (void*)event_base), NULL, fail);
	try(event_add(socket_event, NULL), -1, fail);
	try(event_add(signal_event, NULL), -1, fail);
	try((printf("Server started.\n") < 0), true, fail);
	try(event_base_dispatch(event_base), -1, fail);
	event_free(socket_event);
	event_free(signal_event);
	event_base_free(event_base);
	libevent_global_shutdown();
	try(delete(socket), 1, fail);
	try(tpool_wait(thread_pool), 1, fail);
	try(tpool_destroy(thread_pool), 1, fail);
	free(url);
	return 0;
fail:
	return 1;
}

static void dispatch_request(evutil_socket_t fd, short events, void* arg) {
	struct socket2* listening_socket = arg;
	struct socket2* accepted = socket2_accept(listening_socket);
	tpool_add_work(thread_pool, start_worker, accepted);
}
static void* start_worker(void* arg) {
	struct socket2* socket = arg;
	ft_handler_handle_request(handler, socket);
}

static void connection_listener_close(evutil_socket_t signal, short events, void* user_data) {
	struct event_base* base = user_data;
	struct timeval delay = { 1, 0 };
	printf("\nShutting down...\n");
	event_base_loopexit(base, &delay);
}
