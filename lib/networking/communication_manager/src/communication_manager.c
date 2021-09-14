#include "communication_manager.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#include <event2/event.h>
#include <event2/thread.h>

#include "request_handler.h"
#include "socket2.h"
#include "nproto/nproto_service_ipv4.h"
#include "tproto/tproto_service_tcp.h"
#include "tpool.h"
#include "try.h"
#include "utilities.h"

#define BACKLOG 4096

#ifdef WIN32
#define evthread_use_threads evthread_use_windows_threads
#elif __unix__
#define evthread_use_threads evthread_use_pthreads
#endif

struct cmn_communication_manager {
    cmn_request_handler_t handler;
    cmn_nproto_service_t nproto_service;
    cmn_tproto_service_t tproto_service;
    cmn_socket2_t socket;
    cmn_tpool_t thread_pool;
};

struct hadler_info {
    cmn_request_handler_t handler;
    cmn_socket2_t socket;
};

static void stop(evutil_socket_t signal, short events, void *user_data);

static void dispatch_request(evutil_socket_t fd, short events, void *arg);

static void *handle_request(void *arg);

static struct event_base *event_base;

extern cmn_communication_manager_t cmn_communication_manager_init(size_t thread_number) {
    cmn_communication_manager_t this;
    try(this = malloc(sizeof *this), NULL, fail);
    if (thread_number) {
        try(evthread_use_threads(), -1, fail);
        this->nproto_service = cmn_nproto_service_ipv4;
        this->tproto_service = cmn_tproto_service_tcp;
        try(this->thread_pool = cmn_tpool_init(thread_number), NULL, fail);
    }
    else {
        goto fail;    // TODO: handle mono thread case
    }
    return this;
fail:
    return NULL;
}

extern void cmn_communication_manager_set_nproto(cmn_communication_manager_t this,
                                                 cmn_nproto_service_t nproto_service) {
    this->nproto_service = nproto_service;
}

extern void cmn_communication_manager_set_tproto(cmn_communication_manager_t this,
                                                 cmn_tproto_service_t tproto_serivce) {
    this->tproto_service = tproto_serivce;
}

extern int cmn_communication_manager_start(cmn_communication_manager_t this, const char *url,
                                           cmn_request_handler_t request_handler) {
    struct event *socket_event;
    struct event *signal_event;
    try(this->socket = cmn_socket2_init(this->nproto_service, this->tproto_service), NULL, fail);
    try(cmn_socket2_set_blocking(this->socket, false), 1, fail);
    try(cmn_socket2_listen(this->socket, url, BACKLOG), 1, fail);
    this->handler = request_handler;
    try(event_base = event_base_new(), NULL, fail);
    try(socket_event = event_new(event_base, cmn_socket2_get_fd(this->socket), EV_READ | EV_PERSIST, dispatch_request,
                                 this), NULL, fail);
    try(signal_event = evsignal_new(event_base, SIGINT, stop, (void *) event_base), NULL, fail);
    try(event_add(socket_event, NULL), -1, fail);
    try(event_add(signal_event, NULL), -1, fail);
    try((printf("Server started.\n") < 0), true, fail);
    try(event_base_dispatch(event_base), -1, fail);
    event_free(socket_event);
    event_free(signal_event);
    event_base_free(event_base);
    libevent_global_shutdown();
    try(cmn_socket2_destroy(this->socket), 1, fail);
    return 0;
fail:
    printf("%s\n", strerror(errno));
    return 1;
}

extern int cmn_communication_manager_stop(cmn_communication_manager_t this) {
    stop(0, 0, (void *) event_base);
    return 0;
}

extern int cmn_communication_manager_destroy(cmn_communication_manager_t this) {
    try(cmn_tpool_destroy(this->thread_pool), 1, fail);
    free(this);
    return 0;
fail:
    return 1;
}

static void dispatch_request(evutil_socket_t fd, short events, void *arg) {
    cmn_communication_manager_t this = (cmn_communication_manager_t) arg;
    struct hadler_info *info = malloc(sizeof *info);
    if (info) {
        info->handler = this->handler;
        info->socket = cmn_socket2_accept(this->socket);
        cmn_tpool_add_work(this->thread_pool, handle_request, info);
    }
}

static void *handle_request(void *arg) {
    struct hadler_info *info = arg;
    cmn_request_handler_handle_request(info->handler, info->socket);
    if (info->socket) {
        cmn_socket2_destroy(info->socket);
    }
    free(arg);
    return NULL;
}

static void stop(evutil_socket_t signal, short events, void *user_data) {
    struct event_base *base = user_data;
    struct timeval delay = {1, 0};
    printf("\nShutting down...\n");
    event_base_loopexit(base, &delay);
}
