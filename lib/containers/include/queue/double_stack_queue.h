/**
 * @brief This object inherit from the queue object and is indeed polimorphyc to
 *  it, you can use all the function defined in queue.h casting a
 *  a double_stack_queue object.
 */
#pragma once

#include "queue.h"
#include "stack.h"
#include "stack/linked_list_stack.h"

struct cmn_double_stack_queue {
    struct cmn_queue super;
	struct cmn_linked_list_stack stack_in;
	struct cmn_linked_list_stack stack_out;
};

/**
 * @brief 
 * 
 * @param queue 
 * @return int 
 */
extern int cmn_double_stack_queue_init(struct cmn_double_stack_queue* queue);
