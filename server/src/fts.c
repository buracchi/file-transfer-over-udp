#include "fts.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "ftcp.h"
#include "socket2.h"
#include "tpool.h"
#include "try.h"
#include "utilities.h"

#define BACKLOG 4096

#ifdef WIN32
#define evthread_use_threads evthread_use_windows_threads
#elif __unix__
#define evthread_use_threads evthread_use_pthreads
#endif

static void fts_close(evutil_socket_t signal, short events, void* arg);
static void dispatch_request(evutil_socket_t fd, short events, void* arg);
static void* handle_request(void* arg);
static const char* get_filelist();
static FILE* get_file(const char* filename);
static void save_file(const char* filename);

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
	try(thread_pool = tpool_init(get_nprocs()), NULL, fail);
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
	char* request;
	char* reply;
	socket2_recv(socket, &request);
	switch (ftcp_get_type(request)) {
	case COMMAND:
		switch (ftcp_get_operation(request)) {
		case LIST: {
			reply = (char*) get_filelist();
			socket2_send(socket, reply);
			free(reply);
			break;
		}
		case GET: {
			FILE* file;
			// TODO: fail if request contains the '/' character
			file = get_file(ftcp_get_arg(request));
			// TODO: implement a socket2_fsend(socket2_t, FILE*)
			fclose(file);
			break;
		}
		case PUT: {
			save_file(ftcp_get_arg(request));
			reply = "Success";
			socket2_send(socket, reply);
			break;
		}
		default:
			break;
		}
		break;
	default:
		break;
	}
	socket2_close(socket);
	socket2_destroy(socket);
	free(request);
}

static const char* get_filelist() {
	FILE* pipe;
	char* result;
	try(pipe = popen(list_command, "r"), NULL, fail);
	try(fscanf(pipe, "%m[\x01-\xFF-]", &result), EOF, fail2);
	try(pclose(pipe), -1, fail3);
	return result;
fail3:
	free(result);
fail2:
	pclose(pipe);
fail:
	return NULL;
}

static FILE* get_file(const char* filename) {
	FILE* file;
	char* file_path;
	try(asprintf(&file_path, "%s/%s", base_dir, filename), -1, fail);
	file = fopen(file_path, "r");
	free(file_path);
	return file;
fail:
	return NULL;
}

static void save_file(const char* filename) {
	printf("File saved\n");
}
