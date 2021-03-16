#include "fts.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#include <event2/event.h>
#include "socket2.h"
#include "tpool.h"
#include "try.h"
#include "utilities.h"

static void listen_cb(evutil_socket_t fd, short what, void* arg);
static void write_cb(evutil_socket_t fd, short what, void* arg);
static void signal_cb(evutil_socket_t signal, short events, void* user_data);

static tpool_t tpool;

extern int fts_start(int port, char* pathname) {
	struct event_base* base;
	struct event* socket_event;
	struct event* signal_event;
	socket2_t socket;
	tpool = tpool_init(8);
	try(socket = socket2_init(TCP, IPV4), NULL, fail);
	try(socket2_ipv4_setaddr(socket, "127.0.0.1", port), 1, fail);
	try(socket2_set_blocking(socket, false), 1, fail);
	try(socket2_listen(socket, 256), 1, fail);
	try(base = event_base_new(), NULL, fail);
	try(socket_event = event_new(base, socket2_get_fd(socket), EV_READ | EV_PERSIST, listen_cb, socket), NULL, fail);
	try(signal_event = evsignal_new(base, SIGINT, signal_cb, (void*)base), NULL, fail);
	try(event_add(socket_event, NULL), -1, fail);
	try(event_add(signal_event, NULL), -1, fail);
	printf("Server started.\n");
	event_base_dispatch(base);
	event_free(socket_event);
	event_free(signal_event);
	event_base_free(base);
	libevent_global_shutdown();
	socket2_close(socket);
	socket2_destroy(socket);
	tpool_destroy(tpool);
	return 0;
fail:
	return 1;
}

static void* foo(void* arg) {
	socket2_t socket = arg;
	char* buff;
	socket2_recv(socket, &buff);
	sleep(1);
	printf("%s\n", buff);
	free(buff);
	socket2_close(socket);
	socket2_destroy(socket);
}

static void listen_cb(evutil_socket_t fd, short what, void* arg) {
	socket2_t listener_socket = arg;
	socket2_t accepted;
	accepted = socket2_accept(listener_socket);
	tpool_add_work(tpool, foo, accepted);
}

static void write_cb(evutil_socket_t fd, short what, void* arg) {

}

static void signal_cb(evutil_socket_t signal, short events, void* user_data) {
	struct event_base* base = user_data;
	struct timeval delay = { 2, 0 };
	printf("\nShutting down the server...\n");
	event_base_loopexit(base, &delay);
}
