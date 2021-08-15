#pragma once

#include <stdbool.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_queue {
    struct cmn_queue_vtbl* __ops_vptr;
};

static struct cmn_queue_vtbl {
    int     (*destroy)  (struct cmn_queue* queue);
    bool    (*is_empty) (struct cmn_queue* queue);
    int     (*enqueue)  (struct cmn_queue* queue, void* item);
    void*   (*dequeue)  (struct cmn_queue* queue);
} __cmn_queue_ops_vtbl __attribute__((unused)) = { 0, 0, 0, 0 };

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/**
 * @brief Destroy a queue object.
 * 
 * @param queue the queue to destroy.
 * @return 0 on success; non-zero otherwise. 
 */
static inline int cmn_queue_destroy(struct cmn_queue* queue) {
    return queue->__ops_vptr->destroy(queue);
}

/**
 * @brief Return wether the queue is empty or not.
 * 
 * @param queue the queue to check.
 * @return true if the queue is empty, false otherwise.
 */
static inline bool cmn_queue_is_empty(struct cmn_queue* queue) {
    return queue->__ops_vptr->is_empty(queue);
}

/**
 * @brief Enqueue an item into a queue.
 * 
 * @param queue the queue to enqueue the item into.
 * @param item the item to enqueue into the queue.
 * @return 0 on success; non-zero otherwise. 
 */
static inline int cmn_queue_enqueue(struct cmn_queue* queue, void* item) {
    return queue->__ops_vptr->enqueue(queue, item);
}

/**
 * @brief Dequeue an item out a queue.
 * 
 * @param queue the queue to dequeue the item out.
 * @return void* the pointer that will reference the item.
 */
static inline void* cmn_queue_dequeue(struct cmn_queue* queue) {
    return queue->__ops_vptr->dequeue(queue);
}
