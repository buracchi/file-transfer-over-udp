#include "tpool.h"

#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>

#include "try.h"
#include "queue.h"

typedef struct {
    void* (*routine) (void*);
	void* arg;
} work_t;

struct tpool {
    size_t thread_cnt;
    size_t working_cnt;
    queue_t job_queue;
    pthread_mutex_t mutex;
    pthread_cond_t work_available_cond;
    pthread_cond_t none_working_cond;
    bool stop;
};

static void* worker_routine(void* arg);

#define exist_thread_running(tpool) (tpool->thread_cnt)
#define exist_thread_working(tpool) (tpool->working_cnt)
#define is_job_queue_empty(tpool) queue_is_empty(tpool->job_queue)

extern tpool_t tpool_create(size_t tnumber) {
    struct tpool* tpool;
    try(tpool = malloc(sizeof * tpool), NULL, fail);
    tpool->thread_cnt = tnumber;
    tpool->working_cnt = 0;
    try(tpool->job_queue = queue_init(), NULL, fail2);
    try_pthread_mutex_init(&(tpool->mutex), fail2);
    try_pthread_cond_init(&(tpool->work_available_cond), fail3);
    try_pthread_cond_init(&(tpool->none_working_cond), fail4);
    tpool->stop = false;
    for (int i = 0; i < tnumber; i++) {
        pthread_t thread;
        try_pthread_create(&thread, NULL, worker_routine, tpool, fail6);
        try_pthread_detach(thread, fail6);
    }
    return tpool;
fail6:
    tpool_destroy(tpool);
    return NULL;
fail5:
    try_pthread_cond_destroy(&(tpool->none_working_cond), fail4);
fail4:
    try_pthread_cond_destroy(&(tpool->work_available_cond), fail3);
fail3:
    try_pthread_mutex_destroy(&(tpool->mutex), fail2);
fail2:
    free(tpool);
fail:
    return NULL;
}

extern int tpool_destroy(const tpool_t handle) {
    struct tpool* _this = handle;
    try_pthread_mutex_lock(&(_this->mutex), fail);
    while (!is_job_queue_empty(_this)) {
        free(queue_dequeue(_this->job_queue));
    }
    _this->stop = true;
    try_pthread_cond_broadcast(&(_this->work_available_cond), fail);
    try_pthread_mutex_unlock(&(_this->mutex), fail);
    try(tpool_wait(_this), 1, fail);
    queue_destroy(_this->job_queue);
    try_pthread_mutex_destroy(&(_this->mutex), fail);
    try_pthread_cond_destroy(&(_this->work_available_cond), fail);
    try_pthread_cond_destroy(&(_this->none_working_cond), fail);
    free(_this);
    return 0;
fail:
    return 1;
}

extern int tpool_add_work(const tpool_t handle, void* (*work_routine) (void*), void* arg) {
    struct tpool* _this = handle;
    work_t* work;
    if (work_routine) {
        work = malloc(sizeof * work);
        if (work) {
            work->routine = work_routine;
            work->arg = arg;
            try_pthread_mutex_lock(&(_this->mutex), fail);
            try(queue_enqueue(_this->job_queue, work), !0, fail2);
            try_pthread_cond_broadcast(&(_this->work_available_cond), fail2);
            try_pthread_mutex_unlock(&(_this->mutex), fail);
            return 0;
        }
    }
    return 1;
fail2:
    try_pthread_mutex_unlock(&(_this->mutex), fail);
fail:
    free(work);
    return 1;
}

extern int tpool_wait(const tpool_t handle) {
    struct tpool* _this = handle;
    try_pthread_mutex_lock(&(_this->mutex), fail);
    while (true) {
        if ((!_this->stop && exist_thread_working(_this)) || (_this->stop && exist_thread_running(_this))) {
            try_pthread_cond_wait(&(_this->none_working_cond), &(_this->mutex), fail);
        }
        else {
            break;
        }
    }
    try_pthread_mutex_unlock(&(_this->mutex), fail);
    return 0;
fail:
    return 1;
}

static void* worker_routine(void* arg) {
    struct tpool* tpool = arg;
    work_t* work = NULL;
    while (true) {
        try_pthread_mutex_lock(&(tpool->mutex), exit);
        while (is_job_queue_empty(tpool) && !tpool->stop) {
            try_pthread_cond_wait(&(tpool->work_available_cond), &(tpool->mutex), exit);
        }
        if (tpool->stop) {
            break;
        }
        tpool->working_cnt++;
        if (!is_job_queue_empty(tpool)) {
            work = queue_dequeue(tpool->job_queue);
        }
        try_pthread_mutex_unlock(&(tpool->mutex), exit);
        if (work) {
            work->routine(work->arg);
            free(work);
        }
        try_pthread_mutex_lock(&(tpool->mutex), exit);
        tpool->working_cnt--;
        if (!tpool->stop && !exist_thread_working(tpool) && is_job_queue_empty(tpool)) {
            try_pthread_cond_signal(&(tpool->none_working_cond), exit);
        }
        try_pthread_mutex_unlock(&(tpool->mutex), exit);
    }
    tpool->thread_cnt--;
    try_pthread_cond_signal(&(tpool->none_working_cond), exit);
    try_pthread_mutex_unlock(&(tpool->mutex), exit);
exit:
    return NULL;
}
