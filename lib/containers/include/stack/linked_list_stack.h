/**
 * @brief This object inherit from the stack object and is indeed polimorphyc to
 *  it, you can use all the function defined in stack.h casting a
 *  a simple_stack object.
 * 
 */
#pragma once

#include "list/linked_list.h"

struct cmn_linked_list_stack {
    struct cmn_stack super;
	struct cmn_linked_list list;
};

/**
 * @brief 
 * 
 * @param stack 
 * @return  
 */
extern int cmn_linked_list_stack_init(struct cmn_linked_list_stack* stack);
