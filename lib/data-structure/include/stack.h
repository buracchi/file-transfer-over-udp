#pragma once

#include <stddef.h>
#include <stdbool.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef void* _stack_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/*
* Return an initialized stack object.
*
* @return	the initialized stack on success; NULL otherwise.
*/
extern _stack_t stack_init();

/*
* Destroys the container object. The contained elements are not destroyed.
*
* All iterators, pointers and references are invalidated.
*
* This function never fails.
*
* @param	handle	-	the stack object.
*
* @return	This function returns no value.
*/
extern void stack_destroy(const _stack_t handle);

/*******************************************************************************
*                                Element access                                *
*******************************************************************************/

/*
* Returns reference to the top element in the stack. This is the most recently
* pushed element. This element will be removed on a call to pop().
*
* If the container is not empty, the function never fails.
* Otherwise, it causes undefined behavior.
*
* @param	handle	-	the stack object.
*
* @return	A reference to the first element in the container.
*/
extern void* stack_peek(const _stack_t handle);

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
extern bool stack_is_empty(const _stack_t handle);

/*
* Returns the number of elements in the container.
*
* This function never fails.
*
* @param	handle	-	the stack object.
*
* @return	The number of elements in the container.
*/
extern size_t stack_get_size(const _stack_t handle);

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
extern int stack_push(const _stack_t handle, void* item);

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
extern void* stack_pop(const _stack_t handle);
