#include "tpool.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <threads.h>
#include <semaphore.h>

#include "queue/double_linked_list_stack_queue.h"
#include "try.h"


struct work {
    void *(*routine)(void *);

    void *arg;
};

struct cmn_tpool {
    thrd_t *threads;
    cmn_queue_t work_queue;
    mtx_t work_queue_lock;
    sem_t available_work_semaphore;
    bool shutdown;
    mtx_t shutdown_lock;
};

static int _worker_routine(cmn_tpool_t tpool);

extern cmn_tpool_t cmn_tpool_init(size_t tnumber) {
    cmn_tpool_t this;
    size_t thread_array_size;
    thread_array_size = (tnumber + 1) * sizeof *this->threads;
    try(this = malloc(sizeof *this), NULL, fail);
    memset(this, 0, sizeof *this);
    try(this->threads = malloc(thread_array_size), NULL, fail2);
    memset(this->threads, 0, thread_array_size);
    try(this->work_queue = (cmn_queue_t) cmn_double_linked_list_stack_queue_init(), NULL, fail3);
    try(sem_init(&(this->available_work_semaphore), 0, 0), -1, fail4);
    try(mtx_init(&(this->work_queue_lock), mtx_plain), thrd_error, fail5);
    this->shutdown = false;
    try(mtx_init(&(this->shutdown_lock), mtx_plain), thrd_error, fail6);
    for (size_t i = 0; i < tnumber; i++) {
        thrd_create(this->threads + i, (thrd_start_t) _worker_routine, this);
    }
    return this;
fail6:
    mtx_destroy(&(this->work_queue_lock));
fail5:
    sem_destroy(&(this->available_work_semaphore));
fail4:
    cmn_queue_destroy(this->work_queue);
fail3:
    free(this->threads);
fail2:
    free(this);
fail:
    return NULL;
}

extern int cmn_tpool_destroy(cmn_tpool_t this) {
    try(mtx_lock(&(this->shutdown_lock)), thrd_error, fail);
    this->shutdown = true;
    try(mtx_unlock(&(this->shutdown_lock)), thrd_error, fail);
    // TODO: add an option to empty the queue (i.e. bool cancel_futures)
    try(mtx_lock(&(this->work_queue_lock)), thrd_error, fail);
    while (!cmn_queue_is_empty(this->work_queue)) {
        free(cmn_queue_dequeue(this->work_queue));
    }
    try(mtx_unlock(&(this->work_queue_lock)), thrd_error, fail);
    for (thrd_t *ptr = this->threads; *ptr; ptr++) {
        // Send a wake-up to prevent threads from permanently blocking
        try(sem_post(&(this->available_work_semaphore)), -1, fail);
    }
    for (thrd_t *ptr = this->threads; *ptr; ptr++) {
        try(thrd_join(*ptr, NULL), thrd_error, fail);
    }
    try(cmn_queue_destroy(this->work_queue), 1, fail);
    try(sem_destroy(&(this->available_work_semaphore)), -1, fail);
    mtx_destroy(&(this->work_queue_lock));
    mtx_destroy(&(this->shutdown_lock));
    free(this->threads);
    free(this);
    return 0;
fail:
    return 1;
}

extern int cmn_tpool_add_work(cmn_tpool_t this, void *(*work_routine)(void *), void *arg) {
    struct work *work;
    try(work = malloc(sizeof *work), NULL, fail);
    work->routine = work_routine;
    work->arg = arg;
    try(mtx_lock(&(this->shutdown_lock)), thrd_error, fail2);
    if (this->shutdown) {
        try(mtx_unlock(&(this->shutdown_lock)), thrd_error, fail2);
        free(work);
        return 0;
    }
    try(mtx_lock(&(this->work_queue_lock)), thrd_error, fail3);
    try(cmn_queue_enqueue(this->work_queue, work), !0, fail4);
    try(mtx_unlock(&(this->work_queue_lock)), thrd_error, fail5);
    try(sem_post(&(this->available_work_semaphore)), -1, fail5);
    try(mtx_unlock(&(this->shutdown_lock)), thrd_error, fail);
    return 0;
fail5:
    cmn_queue_dequeue(this->work_queue);
fail4:
    mtx_unlock(&(this->work_queue_lock));
fail3:
    mtx_unlock(&(this->shutdown_lock));
fail2:
    free(work);
fail:
    return 1;
}

static int _worker_routine(cmn_tpool_t tpool) {
    struct work *work;
    while (true) {
        try(sem_wait(&(tpool->available_work_semaphore)), -1, fail);
        if (tpool->shutdown) {
            break;
        }
        try(mtx_lock(&(tpool->work_queue_lock)), thrd_error, fail);
        work = cmn_queue_dequeue(tpool->work_queue);
        try(mtx_unlock(&(tpool->work_queue_lock)), thrd_error, fail);
        work->routine(work->arg);
        free(work);
    }
    return 0;
fail:
    return 1;   // TODO: implement a worker error handler
}
