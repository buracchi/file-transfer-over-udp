#include "worker.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <threads.h>

#include "dispatcher.h"

static int worker_routine(struct worker worker[static 1]);

bool worker_init(struct worker worker[static 1],
                                    size_t id,
                                    size_t max_jobs,
                                    atomic_bool shutdown[static 1],
                                    struct logger logger[static 1]) {
    *worker = (struct worker) {
        .id = id,
        .shutdown = shutdown,
        .jobs = calloc(max_jobs, sizeof *worker->jobs),
        .max_jobs = max_jobs,
        .logger = logger,
    };
    if (worker->jobs == nullptr) {
        logger_log_error(logger, "Could not allocate memory for the jobs array.");
        return false;
    }
    for (size_t i = 0; i < max_jobs; i++) {
        worker->jobs[i].job_id = i;
        worker->jobs[i].dispatcher = &worker->dispatcher;
    }
    if (sem_init(&worker->available_jobs, 0, max_jobs) == -1) {
        logger_log_error(logger, "Could not initialize the handler semaphore. %s", strerror(errno));
        goto fail;
    }
    // Dispatcher should hold up to 1 AIO linked to 1 TIMEOUT and 1 pending TIMEOUT canceled
    if (!dispatcher_init(&worker->dispatcher, max_jobs * 3, logger)) {
        goto fail2;
    }
    if (thrd_create(&worker->thread, (thrd_start_t) worker_routine, worker) != thrd_success) {
        goto fail3;
    }
    return true;
fail3:
    dispatcher_destroy(&worker->dispatcher);
fail2:
    sem_destroy(&worker->available_jobs);
fail:
    free(worker->jobs);
    return false;
}

void worker_destroy(struct worker worker[static 1]) {
    dispatcher_submit(&worker->dispatcher, nullptr);    // wake up the worker
    thrd_join(worker->thread, nullptr);
    dispatcher_destroy(&worker->dispatcher);
    sem_destroy(&worker->available_jobs);
    free(worker->jobs);
}

static int worker_routine(struct worker worker[static 1]) {
    while (!*worker->shutdown || worker->dispatcher.pending_requests != 0) {
        struct dispatcher_event *event;
        if (!dispatcher_wait_event(&worker->dispatcher, &event)) {
            if (errno == EINTR) {
                continue;
            }
            logger_log_fatal(worker->logger, "Worker %d is dead.", worker->id);
            exit(1);
        }
        if (event == nullptr) {
            continue;
        }
        uint16_t sid = event->id >> 48;
        struct worker_job* job = &worker->jobs[sid];
        //logger_log_trace(worker->logger, "Worker %zu received event for session %d.", worker->id, sid);
        switch (job_handle_event(job, event)) {
            case JOB_STATE_ERROR:
                logger_log_fatal(worker->logger, "Worker %zu encountered fatal error", worker->id);
                exit(1);
                break;
            case JOB_STATE_TERMINATED:
                job->is_active = false;
                int ret;
                do {
                    ret = sem_post(&worker->available_jobs);
                } while (ret == -1 && errno == EINTR);
                if (ret == -1) {
                    logger_log_fatal(worker->logger, "Worker %zu encountered fatal error", worker->id);
                    exit(1);
                }
                logger_log_trace(worker->logger, "Worker %zu released handler for session %d.", worker->id, sid);
                break;
            default:
                break;
        }
    }
    return 0;
}
