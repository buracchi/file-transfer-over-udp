#include "worker_pool.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "dispatcher.h"
#include "worker.h"

static struct worker *get_least_busy_worker(struct tftp_server_worker_pool *pool);

bool worker_pool_init(struct tftp_server_worker_pool pool[static 1],
                                  uint16_t workers_number,
                                  uint16_t worker_max_jobs,
                                  struct logger logger[static 1]) {
    *pool = (struct tftp_server_worker_pool) {
        .logger = logger,
        .shutdown = false,
        .workers_number = workers_number,
        .workers = malloc(workers_number * sizeof *pool->workers),
    };
    if (pool->workers == nullptr) {
        return false;
    }
    for (size_t i = 0; i < workers_number; i++) {
        if (!worker_init(&pool->workers[i], i, worker_max_jobs, &pool->shutdown, logger)) {
            for (size_t j = 0; j < i; j++) {
                worker_destroy(&pool->workers[j]);
            }
            free(pool->workers);
            return false;
        }
    }
    return true;
}

bool worker_pool_destroy(struct tftp_server_worker_pool pool[static 1]) {
    pool->shutdown = true;
    for (size_t i = 0; i < pool->workers_number; i++) {
        worker_destroy(&pool->workers[i]);
    }
    free(pool->workers);
    return true;
}

struct worker_job *worker_pool_get_job(struct tftp_server_worker_pool pool[static 1]) {
    struct worker *worker = get_least_busy_worker(pool);
    int ret;
    do {
        ret = sem_wait(&worker->available_jobs);
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) {
        logger_log_fatal(pool->logger, "Worker %zu encountered fatal error. %s.", worker->id, strerror(errno));
        exit(1);
    }
    for (size_t i = 0; i < worker->max_jobs; i++) {
        if (!worker->jobs[i].is_active) {
            return &worker->jobs[i];
        }
    }
    unreachable();
}

bool worker_pool_start_job(struct tftp_server_worker_pool pool[static 1], struct worker_job job[static 1]) {
    logger_log_debug(pool->logger, "Starting session.");
    job->is_active = true;
    if (!dispatcher_submit(job->session.dispatcher, &job->session.event_start)) {
        job->is_active = false;
        return false;
    }
    return true;
}

// TODO implement a reasonable load balancing strategy
static struct worker *get_least_busy_worker(struct tftp_server_worker_pool *pool) {
    size_t least_busy_worker = 0;
    size_t least_busy_worker_queue_size = SIZE_MAX;
    for (size_t i = 0; i < pool->workers_number; i++) {
        int sem_value;
        sem_getvalue(&pool->workers[i].available_jobs, &sem_value);
        size_t busy_queue_size = pool->workers[i].max_jobs - sem_value;
        if (busy_queue_size == 0) {
            least_busy_worker = i;
            break;
        }
        if (busy_queue_size < least_busy_worker_queue_size) {
            least_busy_worker = i;
            least_busy_worker_queue_size = busy_queue_size;
        }
    }
    return &pool->workers[least_busy_worker];
}
