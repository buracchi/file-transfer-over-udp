#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <stdint.h>
#include <threads.h>
#include <semaphore.h>
#include <stdatomic.h>

#include <logger.h>

#include "worker.h"

struct tftp_server_worker_pool {
    struct logger *logger;
    atomic_bool shutdown;
    uint16_t workers_number;
    struct worker *workers;
};

bool worker_pool_init(struct tftp_server_worker_pool pool[static 1],
                                  uint16_t workers_number,
                                  uint16_t worker_max_jobs,
                                  struct logger logger[static 1]);

bool worker_pool_destroy(struct tftp_server_worker_pool pool[static 1]);

struct worker_job *worker_pool_get_job(struct tftp_server_worker_pool pool[static 1]);

bool worker_pool_start_job(struct tftp_server_worker_pool pool[static 1],
                                       struct worker_job job[static 1]);

#endif // WORKER_POOL_H
