#pragma once

#include <stddef.h>
#include <stdbool.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_stack {
    struct cmn_stack_vtbl* __ops_vptr;
};

static struct cmn_stack_vtbl {
    int     (*destroy)  (struct cmn_stack* stack);
    void*   (*peek)     (struct cmn_stack* stack);
    bool    (*is_empty) (struct cmn_stack* stack);
    size_t  (*get_size) (struct cmn_stack* stack);
    int     (*push)     (struct cmn_stack* stack, void* item);
    void*   (*pop)      (struct cmn_stack* stack);
} __cmn_stack_ops_vtbl __attribute__((unused)) = { 0, 0, 0, 0, 0, 0 };

typedef void* _stack_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/**
 * @brief Destroy a stack object.
 * 
 * @details Destroys the container object. The contained elements are not
 *  destroyed.
 *  All iterators, pointers and references are invalidated.
 *
 * @param stack the stack object.
 * @return 0 on success; non-zero otherwise.
 */
static inline int cmn_stack_destroy(struct cmn_stack* stack) {
    return stack->__ops_vptr->destroy(stack);
}

/*******************************************************************************
*                                Element access                                *
*******************************************************************************/

/**
 * @brief Returns the top most element in the stack.
 * 
 * @details Returns reference to the top element in the stack. This is the most
 *  recently pushed element. This element will be removed on a call to pop().
 * 
 * @param stack the stack object.
 * @return A reference to the first element in the container.
 */
static inline void* cmn_stack_peek(struct cmn_stack* stack) {
    return stack->__ops_vptr->peek(stack);
}

/*******************************************************************************
*                                   Capacity                                   *
*******************************************************************************/

/*
* Returns whether the container is empty.
*
* This function never fails.
*
* @param	handle	-	the stack object.
*
* @return	true if the container size is 0, false otherwise.
*/
static inline bool cmn_stack_is_empty(struct cmn_stack* stack) {
    return stack->__ops_vptr->is_empty(stack);
}

/*
* Returns the number of elements in the container.
*
* This function never fails.
*
* @param	handle	-	the stack object.
*
* @return	The number of elements in the container.
*/
static inline size_t cmn_stack_get_size(struct cmn_stack* stack) {
    return stack->__ops_vptr->get_size(stack);
}

/*******************************************************************************
*                                   Modifiers                                  *
*******************************************************************************/

/*
* Pushes the given element value to the top of the stack.
*
* @param	this	-	the stack object.
* @param	value	-	the value of the element to push.
*
* @return	0 on success; non-zero otherwise.
*/
static inline int cmn_stack_push(struct cmn_stack* stack, void* item) {
    return stack->__ops_vptr->push(stack, item);
}

/*
* Removes the top element from the stack.
*
* @param	this	-	the stack object.
* @param	value	-	the pointer that will reference the value of the popped
*						element, if the parameter is NULL the value is ignored.
*
* @return	the pointer that will reference the value of the popped element, if 
*			the parameter is NULL the value is ignored.
*/
static inline void* cmn_stack_pop(struct cmn_stack* stack) {
    return stack->__ops_vptr->pop(stack);
}
