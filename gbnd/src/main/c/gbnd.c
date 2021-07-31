#include "gbnd.h"

#include <stdio.h>
#include <strings.h>
#include <stdbool.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <sys/sysinfo.h>

#include "socket2.h"
#include "nproto/nproto_unix.h"
#include "tproto/tproto_tcp.h"
#include "socket2_proxy.h"
#include "tpool.h"
#include "utilities.h"
#include "try.h"

#define BACKLOG 4096
#define URL "gbnd.ipc.scoket"

#ifdef WIN32
#define evthread_use_threads evthread_use_windows_threads
#define nprocs() 2
#elif __unix__
#define nprocs() get_nprocs()
#define evthread_use_threads evthread_use_pthreads
#endif

static void dispatch_request(evutil_socket_t fd, short events, void* arg);
static void* handle_request(void* arg);

static tpool_t thread_pool;
static struct event_base* event_base;

extern int gbnd_start() {
	struct event* socket_event;
	struct nproto_unix np_unix;
	struct tproto_tcp tp_tcp;
	struct socket2* socket;
	try(thread_pool = tpool_init(nprocs()), NULL, fail);
	try(evthread_use_threads(), -1, fail2);
	try(event_base = event_base_new(), NULL, fail2);
	nproto_unix_init(&np_unix);
	tproto_tcp_init(&tp_tcp);
	try(socket = new(socket2, &tp_tcp.super.tproto, &np_unix.super.nproto, true), NULL, fail2);
	try(socket2_set_blocking(socket, false), 1, fail3);
	try(socket2_listen(socket, URL, BACKLOG), 1, fail3);
	try(socket_event = event_new(event_base, socket2_get_fd(socket), EV_READ | EV_PERSIST, dispatch_request, socket), NULL, fail3);
	try(event_add(socket_event, NULL), -1, fail3);
	try(event_base_dispatch(event_base), -1, fail3);
	event_free(socket_event);
	event_base_free(event_base);
	libevent_global_shutdown();
	try(delete(socket), 1, fail2);
	try(tpool_wait(thread_pool), 1, fail2);
	try(tpool_destroy(thread_pool), 1, fail);
	return 0;
fail3:
	delete(socket);
fail2:
	tpool_destroy(thread_pool);
fail:
	return 1;
}

static void dispatch_request(evutil_socket_t fd, short events, void* arg) {
	struct socket2* listening_socket = arg;
	struct socket2* accepted = socket2_accept(listening_socket);
	tpool_add_work(thread_pool, handle_request, accepted);
}

static void* handle_request(void* arg) {
	struct socket2* socket = arg;
	struct socket2_proxy* proxy;
	socket2_recv(socket, (uint8_t*)&proxy, 1);
	// observe status chenges and add event
	delete(socket);
}
