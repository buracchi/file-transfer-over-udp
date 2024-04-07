#include "uring_thread_pool.h"

#include <stdlib.h>
#include <stddef.h>

#include <liburing.h>
#include <string.h>

static struct uring_worker *uring_thread_pool_get_least_busy_worker(struct uring_thread_pool *pool);
static int worker_routine(struct uring_worker *worker);

void uring_thread_pool_init(struct uring_thread_pool *pool, size_t workers_number, size_t worker_max_sessions, struct logger logger[static 1]) {
    *pool = (struct uring_thread_pool) {
        .logger = logger,
        .shutdown = false,
        .workers_number = workers_number,
        .worker_max_sessions = worker_max_sessions,
    };
    for (size_t i = 0; i < workers_number; i++) {
        pool->workers[i] = (struct uring_worker) {
            .handlers = calloc(worker_max_sessions, sizeof *(*pool->workers).handlers),
            .handlers_busy_mask = calloc(worker_max_sessions, sizeof *(*pool->workers).handlers_busy_mask),
        };
        if (pool->workers[i].handlers == nullptr) {
            logger_log_error(pool->logger, "Could not allocate memory for the handlers array.");
            exit(EXIT_FAILURE);
        }
        if (pool->workers[i].handlers_busy_mask == nullptr) {
            logger_log_error(pool->logger, "Could not allocate memory for the handlers busy mask.");
            exit(EXIT_FAILURE);
        }
        int ret = sem_init(&pool->workers[i].available_handlers, 0, pool->worker_max_sessions);
        if (ret == -1) {
            logger_log_error(pool->logger, "Could not initialize the handler semaphore. %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        ret = io_uring_queue_init(worker_max_sessions * 2, &pool->workers[i].ring, 0);
        if (ret < 0) {
            exit(EXIT_FAILURE);
        }
        struct io_uring_sqe *sqe = io_uring_get_sqe(&pool->workers[i].ring);
        if (sqe == nullptr) {
            logger_log_error(pool->logger, "Could not get a sqe from the ring.");
            exit(EXIT_FAILURE);
        }
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data(sqe, pool);
        ret = io_uring_submit(&pool->workers[i].ring);
        if (ret < 0) {
            logger_log_error(pool->logger, "Could not submit a NOP request to the ring. %s", strerror(-ret));
            exit(EXIT_FAILURE);
        }
        thrd_create(&pool->workers[i].worker, (thrd_start_t) worker_routine, &pool->workers[i]);
    }
}

// TODO: add an option to empty the queue (i.e. bool cancel_futures)
int uring_thread_pool_destroy(struct uring_thread_pool *pool) {
    pool->shutdown = true;
    for (size_t i = 0; i < pool->workers_number; i++) {
        // wake up workers
        struct io_uring_sqe *sqe = io_uring_get_sqe(&pool->workers[i].ring);
        io_uring_prep_nop(sqe);
        io_uring_sqe_set_data(sqe, nullptr);
        int ret = io_uring_submit(&pool->workers[i].ring);
        if (ret < 0) {
            logger_log_error(pool->logger, "Could not submit a NOP request to the ring. %s", strerror(-ret));
            exit(EXIT_FAILURE);
        }
    }
    for (size_t i = 0; i < pool->workers_number; i++) {
        thrd_join(pool->workers[i].worker, nullptr);
        io_uring_queue_exit(&pool->workers[i].ring);
        sem_destroy(&pool->workers[i].available_handlers);
        free(pool->workers[i].handlers);
        free(pool->workers[i].handlers_busy_mask);
    }
    return 0;
}

struct uring_worker_tftp_handler *uring_thread_pool_get_available_worker_handler(struct uring_thread_pool *pool) {
    struct uring_worker *worker = uring_thread_pool_get_least_busy_worker(pool);
    sem_wait(&worker->available_handlers);
    for (size_t i = 0; i < pool->worker_max_sessions; i++) {
        if (!worker->handlers_busy_mask[i]) {
            worker->handlers_busy_mask[i] = true;
            memset(&worker->handlers[i], 0, sizeof *worker->handlers);
            worker->handlers[i].worker = worker;
            return &worker->handlers[i];
        }
    }
    return nullptr;
}

void uring_thread_pool_after_handler_closed(struct tftp_handler handler[static 1]) {
    struct uring_worker_tftp_handler *worker_handler = (struct uring_worker_tftp_handler *) handler;
    size_t worker_handler_index = worker_handler - worker_handler->worker->handlers;
    worker_handler->worker->handlers_busy_mask[worker_handler_index] = false;
    sem_post(&worker_handler->worker->available_handlers);
}

// TODO implement a reasonable load balancing strategy
static struct uring_worker *uring_thread_pool_get_least_busy_worker(struct uring_thread_pool *pool) {
    size_t least_busy_worker = 0;
    size_t least_busy_worker_queue_size = SIZE_MAX;
    for (size_t i = 0; i < pool->workers_number; i++) {
        int sem_value;
        sem_getvalue(&pool->workers[i].available_handlers, &sem_value);
        size_t busy_queue_size = pool->worker_max_sessions - sem_value;
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

// TODO: io_uring_wait_cqe ret < 0
static int worker_routine(struct uring_worker *worker) {
    struct uring_thread_pool *pool = nullptr;
    struct io_uring_cqe *cqe = nullptr;
    int ret = io_uring_wait_cqe(&worker->ring, &cqe);
    if (ret < 0) {
        logger_log_error(pool->logger, "io_uring_wait_cqe failed: %s", strerror(-ret));
        exit(1);
    }
    pool = (struct uring_thread_pool *) io_uring_cqe_get_data(cqe);
    io_uring_cqe_seen(&worker->ring, cqe);
    // size_t worker_id = worker - pool->workers;
    while (true) {
        if (pool->shutdown) {
            break;
        }
        do {
            ret = io_uring_wait_cqe(&worker->ring, &cqe);
        } while (ret == -EINTR);
        if (ret < 0) {
            logger_log_error(pool->logger, "io_uring_wait_cqe failed: %s", strerror(-ret));
            exit(1);
        }
        struct uring_job *job = io_uring_cqe_get_data(cqe);
        io_uring_cqe_seen(&worker->ring, cqe);
        if (job != nullptr) {
            job->routine(cqe->res, job->args);
        }
    }
    return 0;
}
