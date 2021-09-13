#pragma once

#include <stdbool.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_queue *cmn_queue_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/**
 * @brief Destroy a queue object.
 * 
 * @param queue the queue to destroy.
 * @return 0 on success; non-zero otherwise. 
 */
extern int cmn_queue_destroy(cmn_queue_t queue);

/**
 * @brief Return wether the queue is empty or not.
 * 
 * @param queue the queue to check.
 * @return true if the queue is empty, false otherwise.
 */
extern bool cmn_queue_is_empty(cmn_queue_t queue);

/**
 * @brief Enqueue an item into a queue.
 * 
 * @param queue the queue to enqueue the item into.
 * @param item the item to enqueue into the queue.
 * @return 0 on success; non-zero otherwise. 
 */
extern int cmn_queue_enqueue(cmn_queue_t queue, void *item);

/**
 * @brief Dequeue an item out a queue.
 * 
 * @param queue the queue to dequeue the item out.
 * @return void* the pointer that will reference the item.
 */
extern void *cmn_queue_dequeue(cmn_queue_t queue);
