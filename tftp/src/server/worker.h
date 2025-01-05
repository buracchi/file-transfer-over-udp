#ifndef WORKER_H
#define WORKER_H

#include <semaphore.h>
#include <stdatomic.h>
#include <stddef.h>
#include <threads.h>

#include <logger.h>

#include "dispatcher.h"
#include "worker_job.h"

struct worker {
    uint16_t id;
    atomic_bool *shutdown;
    thrd_t thread;
    uint16_t max_jobs;
    struct dispatcher dispatcher;
    sem_t available_jobs;
    struct logger *logger;
    struct worker_job *jobs;
};

bool worker_init(struct worker worker[static 1],
                 size_t id,
                 size_t max_jobs,
                 atomic_bool shutdown[static 1],
                 struct logger logger[static 1]);

void worker_destroy(struct worker worker[static 1]);

#endif // WORKER_H
