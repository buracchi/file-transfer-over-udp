#pragma once

#include <threads.h>
#include <semaphore.h>
#include <stdatomic.h>

#include <liburing.h>
#include <logger.h>
#include <tftp.h>

#include "uring_job.h"
#include "tftp_handler.h"


// TODO require a logger for each worker

struct uring_worker {
    thrd_t worker;
    struct io_uring ring;
    sem_t available_handlers;
    bool *handlers_busy_mask;
    struct uring_worker_tftp_handler {
        struct tftp_handler handler;
        struct uring_worker *worker;
        struct uring_job start_job;
        struct new_tftp_request_args {
            struct tftp_server *server;
            struct sockaddr_storage peer_addr;
            socklen_t peer_addrlen;
            ssize_t bytes_recvd;
            bool is_orig_dest_addr_ipv4;
            uint8_t buffer[tftp_request_packet_max_size];
        } new_request_args;
    } *handlers;    // pointer to array of worker_max_sessions elements
};

struct uring_thread_pool {
    struct logger *logger;
    atomic_bool shutdown;
    size_t workers_number;
    size_t worker_max_sessions;
    struct uring_worker workers[];
};

void uring_thread_pool_init(struct uring_thread_pool *pool, size_t workers_number, size_t worker_max_sessions, struct logger logger[static 1]);

int uring_thread_pool_destroy(struct uring_thread_pool *pool);

struct uring_worker_tftp_handler *uring_thread_pool_get_available_worker_handler(struct uring_thread_pool *pool);

void uring_thread_pool_after_handler_closed(struct tftp_handler handler[static 1]);
