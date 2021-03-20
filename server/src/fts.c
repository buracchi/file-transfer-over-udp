#include "fts.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "ftcp.h"
#include "socket2.h"
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

static void fts_close(evutil_socket_t signal, short events, void* arg);
static void dispatch_request(evutil_socket_t fd, short events, void* arg);
static void* handle_request(void* arg);
static void handle_list_request(socket2_t socket, ftcp_pp_t request);
static void handle_get_request(socket2_t socket, ftcp_pp_t request);
static void handle_put_request(socket2_t socket, ftcp_pp_t request);
static void handle_invalid_request(socket2_t socket, ftcp_pp_t request);

static tpool_t thread_pool;
static const char* base_dir;
static const char* list_command;
static struct event_base* event_base;

extern int fts_start(int port, char* pathname) {
	struct event* socket_event;
	struct event* signal_event;
	socket2_t socket;
	base_dir = pathname;
	try(asprintf((char**)&list_command, "ls %s -p | grep -v /", base_dir), -1, fail);
	try(thread_pool = tpool_init(nprocs()), NULL, fail);
	try(evthread_use_threads(), -1, fail);
	try(event_base = event_base_new(), NULL, fail);
	try(socket = socket2_init(TCP, IPV4), NULL, fail);
	try(socket2_ipv4_setaddr(socket, "127.0.0.1", port), 1, fail);
	try(socket2_set_blocking(socket, false), 1, fail);
	try(socket2_listen(socket, BACKLOG), 1, fail);
	try(socket_event = event_new(event_base, socket2_get_fd(socket), EV_READ | EV_PERSIST, dispatch_request, socket), NULL, fail);
	try(signal_event = evsignal_new(event_base, SIGINT, fts_close, (void*)event_base), NULL, fail);
	try(event_add(socket_event, NULL), -1, fail);
	try(event_add(signal_event, NULL), -1, fail);
	try((printf("Server started.\n") < 0), true, fail);
	try(event_base_dispatch(event_base), -1, fail);
	event_free(socket_event);
	event_free(signal_event);
	event_base_free(event_base);
	libevent_global_shutdown();
	try(socket2_close(socket), 1, fail);
	socket2_destroy(socket);
	try(tpool_wait(thread_pool), 1, fail);
	try(tpool_destroy(thread_pool), 1, fail);
	free((char*)list_command);
	return 0;
fail:
	return 1;
}

static void fts_close(evutil_socket_t signal, short events, void* user_data) {
	struct event_base* base = user_data;
	struct timeval delay = { 1, 0 };
	printf("\nShutting down the server...\n");
	event_base_loopexit(base, &delay);
}

static void dispatch_request(evutil_socket_t fd, short events, void* arg) {
	socket2_t listening_socket = arg;
	socket2_t accepted = socket2_accept(listening_socket);
	tpool_add_work(thread_pool, handle_request, accepted);
}

static void* handle_request(void* arg) {
	socket2_t socket = arg;
	ftcp_pp_t request = malloc(ftcp_pp_size());
	socket2_recv(socket, request, ftcp_pp_size());
	switch (ftcp_get_type(request)) {
	case COMMAND:
		switch (ftcp_get_operation(request)) {
		case LIST:
			handle_list_request(socket, request);
			break;
		case GET:
			handle_get_request(socket, request);
			break;
		case PUT:
			handle_put_request(socket, request);
			break;
		default:
			handle_invalid_request(socket, request);
		}
		break;
	default:
		handle_invalid_request(socket, request);
	}
	socket2_close(socket);
	socket2_destroy(socket);
	free(request);
}

static void handle_list_request(socket2_t socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	FILE* pipe;
	char* filelist;
	pipe = popen(list_command, "r");
	fscanf(pipe, "%m[\x01-\xFF-]", &filelist);
	pclose(pipe);
	reply = ftcp_pp_init(RESPONSE, SUCCESS, NULL, strlen(filelist));
	socket2_send(socket, reply, ftcp_pp_size());
	socket2_ssend(socket, filelist);
	free(filelist);
	free(reply);
}

// TODO: fail if filename contains the '/' character
static void handle_get_request(socket2_t socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	char* filepath;
	FILE* file;
	uint64_t flen;
	asprintf(&filepath, "%s/%s", base_dir, ftcp_get_arg(request));
	if (!access(filepath, F_OK)) {
		file = fopen(filepath, "r");
		fseek(file, 0L, SEEK_END);
		flen = ftell(file);
		fseek(file, 0L, SEEK_SET);
		reply = ftcp_pp_init(RESPONSE, FILE_EXIST, ftcp_get_arg(request), flen);
		socket2_send(socket, reply, ftcp_pp_size());
		socket2_fsend(socket, file);
		fclose(file);
	}
	else {
		reply = ftcp_pp_init(RESPONSE, FILE_NOT_EXIST, NULL, 0);
		socket2_send(socket, reply, ftcp_pp_size());
	}
	free(filepath);
	free(reply);
}

static void handle_put_request(socket2_t socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	FILE* file;
	char* filepath;
	enum ftcp_result result;
	asprintf(&filepath, "%s/%s", base_dir, ftcp_get_arg(request));
	result = access(filepath, F_OK) ? FILE_NOT_EXIST : FILE_EXIST;
	file = fopen(filepath, "w");
	reply = ftcp_pp_init(RESPONSE, result, NULL, 0);
	socket2_send(socket, reply, ftcp_pp_size());
	free(reply);
	socket2_frecv(socket, file, ftcp_get_dplen(request));
	reply = ftcp_pp_init(RESPONSE, SUCCESS, NULL, 0);
	socket2_send(socket, reply, ftcp_pp_size());
	fclose(file);
	free(filepath);
	free(reply);
}

static void handle_invalid_request(socket2_t socket, ftcp_pp_t request) {
	ftcp_pp_t reply;
	reply = ftcp_pp_init(RESPONSE, ERROR, NULL, 0);
	socket2_send(socket, reply, ftcp_pp_size());
	free(reply);
}
