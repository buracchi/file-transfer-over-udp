#pragma once

#include <stddef.h>
#include <stdbool.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_stack *cmn_stack_t;

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
extern int cmn_stack_destroy(cmn_stack_t stack);

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
extern void *cmn_stack_peek(cmn_stack_t stack);

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
extern bool cmn_stack_is_empty(cmn_stack_t stack);

/*
* Returns the number of elements in the container.
*
* This function never fails.
*
* @param	handle	-	the stack object.
*
* @return	The number of elements in the container.
*/
extern size_t cmn_stack_get_size(cmn_stack_t stack);

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
extern int cmn_stack_push(cmn_stack_t stack, void *item);

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
extern void *cmn_stack_pop(cmn_stack_t stack);
