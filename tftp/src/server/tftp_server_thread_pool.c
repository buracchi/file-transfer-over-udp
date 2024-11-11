#include <buracchi/tftp/server_thread_pool.h>

#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <threads.h>

#include <buracchi/tftp/dispatcher.h>

static bool tftp_server_worker_init(struct tftp_server_worker worker[static 1],
                                    size_t id,
                                    size_t worker_max_sessions,
                                    atomic_bool shutdown[static 1],
                                    struct tftp_server_info server_info[static 1],
                                    struct logger logger[static 1]);

static void tftp_server_worker_destroy(struct tftp_server_worker worker[static 1]);

static struct tftp_server_worker *get_least_busy_worker(struct tftp_server_thread_pool *pool);

static int worker_routine(struct tftp_server_worker worker[static 1]);

bool tftp_server_thread_pool_init(struct tftp_server_thread_pool pool[static 1],
                                  uint16_t workers_number,
                                  uint16_t worker_max_sessions,
                                  struct tftp_server_info server_info[static 1],
                                  struct logger logger[static 1]) {
    *pool = (struct tftp_server_thread_pool) {
        .logger = logger,
        .shutdown = false,
        .workers_number = workers_number,
        .workers = malloc(workers_number * sizeof *pool->workers),
        .server_info = *server_info,
    };
    if (pool->workers == nullptr) {
        return false;
    }
    for (size_t i = 0; i < workers_number; i++) {
        if (!tftp_server_worker_init(&pool->workers[i], i, worker_max_sessions, &pool->shutdown, &pool->server_info, logger)) {
            for (size_t j = 0; j < i; j++) {
                tftp_server_worker_destroy(&pool->workers[j]);
            }
            free(pool->workers);
            return false;
        }
    }
    return true;
}

bool tftp_server_thread_pool_destroy(struct tftp_server_thread_pool pool[static 1]) {
    pool->shutdown = true;
    for (size_t i = 0; i < pool->workers_number; i++) {
        tftp_server_worker_destroy(&pool->workers[i]);
    }
    return true;
}

struct tftp_session *tftp_server_thread_pool_new_session(struct tftp_server_thread_pool pool[static 1]) {
    struct tftp_server_worker *worker = get_least_busy_worker(pool);
    int ret;
    do {
        ret = sem_wait(&worker->available_sessions);
    } while (ret == -1 && errno == EINTR);
    if (ret == -1) {
        logger_log_fatal(pool->logger, "Worker %zu encountered fatal error. %s.", worker->id, strerror(errno));
        exit(1);
    }
    for (size_t i = 0; i < worker->max_sessions; i++) {
        if (!worker->sessions[i].is_active) {
            worker->sessions[i].is_active = true;
            return &worker->sessions[i];
        }
    }
    logger_log_fatal(pool->logger, "This should not happen. Handler available token was acquired but no handler was available.");
    exit(1);
}

static bool tftp_server_worker_init(struct tftp_server_worker worker[static 1],
                                    size_t id,
                                    size_t worker_max_sessions,
                                    atomic_bool shutdown[static 1],
                                    struct tftp_server_info server_info[static 1],
                                    struct logger logger[static 1]) {
    *worker = (struct tftp_server_worker) {
            .id = id,
            .shutdown = shutdown,
            .sessions = calloc(worker_max_sessions, sizeof *worker->sessions),
            .max_sessions = worker_max_sessions,
            .logger = logger,
    };
    if (worker->sessions == nullptr) {
        logger_log_error(logger, "Could not allocate memory for the sessions array.");
        return false;
    }
    for (size_t i = 0; i < worker_max_sessions; i++) {
        tftp_session_init(&worker->sessions[i], i, server_info, &worker->dispatcher, worker->logger);
    }
    if (sem_init(&worker->available_sessions, 0, worker_max_sessions) == -1) {
        logger_log_error(logger, "Could not initialize the handler semaphore. %s", strerror(errno));
        goto fail;
    }
    // Dispatcher should hold up to 1 AIO linked to 1 TIMEOUT and 1 pending TIMEOUT canceled
    if (!dispatcher_init(&worker->dispatcher, worker_max_sessions * 3, logger)) {
        goto fail2;
    }
    if (thrd_create(&worker->thread, (thrd_start_t) worker_routine, worker) != thrd_success) {
        goto fail3;
    }
    return true;
fail3:
    dispatcher_destroy(&worker->dispatcher);
fail2:
    sem_destroy(&worker->available_sessions);
fail:
    free(worker->sessions);
    return false;
}

static void tftp_server_worker_destroy(struct tftp_server_worker worker[static 1]) {
    dispatcher_submit(&worker->dispatcher, nullptr);    // wake up the worker
    thrd_join(worker->thread, nullptr);
    dispatcher_destroy(&worker->dispatcher);
    sem_destroy(&worker->available_sessions);
    free(worker->sessions);
}

// TODO implement a reasonable load balancing strategy
static struct tftp_server_worker *get_least_busy_worker(struct tftp_server_thread_pool *pool) {
    size_t least_busy_worker = 0;
    size_t least_busy_worker_queue_size = SIZE_MAX;
    for (size_t i = 0; i < pool->workers_number; i++) {
        int sem_value;
        sem_getvalue(&pool->workers[i].available_sessions, &sem_value);
        size_t busy_queue_size = pool->workers[i].max_sessions - sem_value;
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

static int worker_routine(struct tftp_server_worker worker[static 1]) {
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
        logger_log_trace(worker->logger, "Worker %zu received event for session %d.", worker->id, sid);
        switch (tftp_session_on_event(&worker->sessions[sid], event)) {
            case TFTP_SESSION_ERROR:
                logger_log_fatal(worker->logger, "Worker %zu encountered fatal error", worker->id);
                exit(1);
                break;
            case TFTP_SESSION_CLOSED:
                int ret;
                do {
                    ret = sem_post(&worker->available_sessions);
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
