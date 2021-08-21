#include "tpool.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/types.h>

#include "queue/double_linked_list_stack_queue.h"
#include "try.h"
#include "utilities.h"

struct work {
    void* (*routine) (void*);
	void* arg;
};

struct cmn_tpool {
    size_t thread_cnt;
    size_t working_cnt;
    cmn_queue_t job_queue;
    pthread_mutex_t mutex;
    pthread_cond_t work_available_cond;
    pthread_cond_t none_working_cond;
    bool stop;
};

static void* worker_routine(void* arg);

#define exist_thread_running(tpool) (tpool->thread_cnt)
#define exist_thread_working(tpool) (tpool->working_cnt)
#define is_job_queue_empty(tpool) cmn_queue_is_empty(tpool->job_queue)

extern cmn_tpool_t cmn_tpool_init(size_t tnumber) {
    cmn_tpool_t this;
    try(this = malloc(sizeof *this), NULL, fail);
    memset(this, 0, sizeof *this);
    this->thread_cnt = tnumber;
    this->working_cnt = 0;
    try(this->job_queue = (cmn_queue_t)cmn_double_linked_list_stack_queue_init(), NULL, fail);
    try_pthread_mutex_init(&(this->mutex), fail2);
    try_pthread_cond_init(&(this->work_available_cond), fail3);
    try_pthread_cond_init(&(this->none_working_cond), fail4);
    this->stop = false;
    pthread_t thread;
    for (size_t i = 0; i < tnumber; i++) {
        try_pthread_create(&thread, NULL, worker_routine, this, fail5);
        try_pthread_detach(thread, fail6);
    }
    return this;
fail6:
    pthread_join(thread, NULL);
fail5:
    pthread_cond_destroy(&(this->none_working_cond));
fail4:
    pthread_cond_destroy(&(this->work_available_cond));
fail3:
    pthread_mutex_destroy(&(this->mutex));
fail2:
    cmn_queue_destroy(this->job_queue);
fail:
    return NULL;
}

extern int cmn_tpool_destroy(cmn_tpool_t this) {
    try_pthread_mutex_lock(&(this->mutex), fail);
    while (!is_job_queue_empty(this)) {
        free(cmn_queue_dequeue(this->job_queue));
    }
    this->stop = true;
    try_pthread_cond_broadcast(&(this->work_available_cond), fail);
    try_pthread_mutex_unlock(&(this->mutex), fail);
    try(cmn_tpool_wait(this), 1, fail);
    cmn_queue_destroy(this->job_queue);
    try_pthread_mutex_destroy(&(this->mutex), fail);
    try_pthread_cond_destroy(&(this->work_available_cond), fail);
    try_pthread_cond_destroy(&(this->none_working_cond), fail);
    free(this);
    return 0;
fail:
    return 1;
}

extern int cmn_tpool_add_work(cmn_tpool_t this, void* (*work_routine) (void*), void* arg) {
    if (this->stop) {
        return 1;
    }
    struct work* work;
    work = malloc(sizeof * work);
    if (work) {
        work->routine = work_routine;
        work->arg = arg;
        try_pthread_mutex_lock(&(this->mutex), fail);
        try(cmn_queue_enqueue(this->job_queue, work), !0, fail2);
        try_pthread_cond_broadcast(&(this->work_available_cond), fail3);
        try_pthread_mutex_unlock(&(this->mutex), fail3);
        return 0;
    }
fail3:
    cmn_queue_dequeue(this->job_queue);
fail2:
    pthread_mutex_unlock(&(this->mutex));
fail:
    free(work);
    return 1;
}

extern int cmn_tpool_wait(cmn_tpool_t this) {
    try_pthread_mutex_lock(&(this->mutex), fail);
    for (;;) {
        if ((!this->stop && exist_thread_working(this)) || (this->stop && exist_thread_running(this))) {
            try_pthread_cond_wait(&(this->none_working_cond), &(this->mutex), fail);
        }
        else {
            break;
        }
    }
    try_pthread_mutex_unlock(&(this->mutex), fail);
    return 0;
fail:
    return 1;
}

static void* worker_routine(void* arg) {
    cmn_tpool_t this = arg;
    struct work* work = NULL;
    for (;;) {
        try_pthread_mutex_lock(&(this->mutex), fail);
        while (is_job_queue_empty(this) && !this->stop) {
            try_pthread_cond_wait(&(this->work_available_cond), &(this->mutex), fail);
        }
        if (this->stop) {
            break;
        }
        this->working_cnt++;
        if (!is_job_queue_empty(this)) {
            work = cmn_queue_dequeue(this->job_queue);
        }
        try_pthread_mutex_unlock(&(this->mutex), fail);
        work->routine(work->arg);
        free(work);
        try_pthread_mutex_lock(&(this->mutex), fail);
        this->working_cnt--;
        if (!this->stop && !exist_thread_working(this) && is_job_queue_empty(this)) {
            try_pthread_cond_signal(&(this->none_working_cond), fail);
        }
        try_pthread_mutex_unlock(&(this->mutex), fail);
    }
    this->thread_cnt--;
    try_pthread_cond_signal(&(this->none_working_cond), fail);
    try_pthread_mutex_unlock(&(this->mutex), fail);
fail:
    return NULL;
}
