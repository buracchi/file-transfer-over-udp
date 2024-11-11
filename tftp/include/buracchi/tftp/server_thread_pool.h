#ifndef TFTP_SERVER_THREAD_POOL_H
#define TFTP_SERVER_THREAD_POOL_H

#include <stdint.h>
#include <threads.h>
#include <semaphore.h>
#include <stdatomic.h>

#include <logger.h>

#include <buracchi/tftp/dispatcher.h>
#include <buracchi/tftp/server_session.h>

struct tftp_server_worker {
    uint16_t id;
    atomic_bool *shutdown;
    thrd_t thread;
    uint16_t max_sessions;
    struct dispatcher dispatcher;
    sem_t available_sessions;
    struct logger *logger;
    struct tftp_session *sessions;
};

struct tftp_server_thread_pool {
    struct logger *logger;
    atomic_bool shutdown;
    struct tftp_server_info server_info;
    uint16_t workers_number;
    struct tftp_server_worker *workers;
};

bool tftp_server_thread_pool_init(struct tftp_server_thread_pool pool[static 1],
                                  uint16_t workers_number,
                                  uint16_t worker_max_sessions,
                                  struct tftp_server_info server_info[static 1],
                                  struct logger logger[static 1]);

bool tftp_server_thread_pool_destroy(struct tftp_server_thread_pool pool[static 1]);

struct tftp_session *tftp_server_thread_pool_new_session(struct tftp_server_thread_pool pool[static 1]);

#endif // TFTP_SERVER_THREAD_POOL_H
