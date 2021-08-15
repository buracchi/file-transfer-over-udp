#include "tpool.h"

#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "try.h"
#include "utilities.h"

struct work {
    void* (*routine) (void*);
	void* arg;
};

static void* worker_routine(void* arg);

#define exist_thread_running(tpool) (tpool->thread_cnt)
#define exist_thread_working(tpool) (tpool->working_cnt)
#define is_job_queue_empty(tpool) cmn_queue_is_empty(&(tpool->job_queue.super))

extern int cmn_tpool_init(struct cmn_tpool* tpool, size_t tnumber) {
    memset(tpool, 0, sizeof *tpool);
    tpool->thread_cnt = tnumber;
    tpool->working_cnt = 0;
    try(cmn_double_stack_queue_init(&(tpool->job_queue)), !0, fail);
    try_pthread_mutex_init(&(tpool->mutex), fail2);
    try_pthread_cond_init(&(tpool->work_available_cond), fail3);
    try_pthread_cond_init(&(tpool->none_working_cond), fail4);
    tpool->stop = false;
    pthread_t thread;
    for (size_t i = 0; i < tnumber; i++) {
        try_pthread_create(&thread, NULL, worker_routine, tpool, fail5);
        try_pthread_detach(thread, fail6);
    }
    return 0;
fail6:
    pthread_join(thread, NULL);
fail5:
    pthread_cond_destroy(&(tpool->none_working_cond));
fail4:
    pthread_cond_destroy(&(tpool->work_available_cond));
fail3:
    pthread_mutex_destroy(&(tpool->mutex));
fail2:
    cmn_queue_destroy(&(tpool->job_queue.super));
fail:
    return 1;
}

extern int cmn_tpool_destroy(struct cmn_tpool* tpool) {
    try_pthread_mutex_lock(&(tpool->mutex), fail);
    while (!is_job_queue_empty(tpool)) {
        free(cmn_queue_dequeue(&(tpool->job_queue.super)));
    }
    tpool->stop = true;
    try_pthread_cond_broadcast(&(tpool->work_available_cond), fail);
    try_pthread_mutex_unlock(&(tpool->mutex), fail);
    try(cmn_tpool_wait(tpool), 1, fail);
    cmn_queue_destroy(&(tpool->job_queue.super));
    try_pthread_mutex_destroy(&(tpool->mutex), fail);
    try_pthread_cond_destroy(&(tpool->work_available_cond), fail);
    try_pthread_cond_destroy(&(tpool->none_working_cond), fail);
    return 0;
fail:
    return 1;
}

extern int cmn_tpool_add_work(struct cmn_tpool* tpool, void* (*work_routine) (void*), void* arg) {
    if (tpool->stop) {
        return 1;
    }
    struct work* work;
    work = malloc(sizeof * work);
    if (work) {
        work->routine = work_routine;
        work->arg = arg;
        try_pthread_mutex_lock(&(tpool->mutex), fail);
        try(cmn_queue_enqueue(&(tpool->job_queue.super), work), !0, fail2);
        try_pthread_cond_broadcast(&(tpool->work_available_cond), fail3);
        try_pthread_mutex_unlock(&(tpool->mutex), fail3);
        return 0;
    }
fail3:
    cmn_queue_dequeue(&(tpool->job_queue.super));
fail2:
    pthread_mutex_unlock(&(tpool->mutex));
fail:
    free(work);
    return 1;
}

extern int cmn_tpool_wait(struct cmn_tpool* tpool) {
    try_pthread_mutex_lock(&(tpool->mutex), fail);
    for (;;) {
        if ((!tpool->stop && exist_thread_working(tpool)) || (tpool->stop && exist_thread_running(tpool))) {
            try_pthread_cond_wait(&(tpool->none_working_cond), &(tpool->mutex), fail);
        }
        else {
            break;
        }
    }
    try_pthread_mutex_unlock(&(tpool->mutex), fail);
    return 0;
fail:
    return 1;
}

static void* worker_routine(void* arg) {
    struct cmn_tpool* tpool = arg;
    struct work* work = NULL;
    for (;;) {
        try_pthread_mutex_lock(&(tpool->mutex), fail);
        while (is_job_queue_empty(tpool) && !tpool->stop) {
            try_pthread_cond_wait(&(tpool->work_available_cond), &(tpool->mutex), fail);
        }
        if (tpool->stop) {
            break;
        }
        tpool->working_cnt++;
        if (!is_job_queue_empty(tpool)) {
            work = cmn_queue_dequeue(&(tpool->job_queue.super));
        }
        try_pthread_mutex_unlock(&(tpool->mutex), fail);
        work->routine(work->arg);
        free(work);
        try_pthread_mutex_lock(&(tpool->mutex), fail);
        tpool->working_cnt--;
        if (!tpool->stop && !exist_thread_working(tpool) && is_job_queue_empty(tpool)) {
            try_pthread_cond_signal(&(tpool->none_working_cond), fail);
        }
        try_pthread_mutex_unlock(&(tpool->mutex), fail);
    }
    tpool->thread_cnt--;
    try_pthread_cond_signal(&(tpool->none_working_cond), fail);
    try_pthread_mutex_unlock(&(tpool->mutex), fail);
fail:
    return NULL;
}
