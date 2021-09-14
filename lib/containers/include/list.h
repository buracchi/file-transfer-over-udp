#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "iterator.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_list *cmn_list_t;

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/*
* Destroys the container object. The contained elements are not destroyed.
*
* All iterators, pointers and references are invalidated.
*
* This function never fails.
*
* @param	list	-	the list object.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_list_destroy(cmn_list_t list);

/*******************************************************************************
*                                Element access                                *
*******************************************************************************/

/*
* Returns a direct reference to the first element in the list container.
*
* If the container is not empty, the function never fails.
* Otherwise, it causes undefined behavior.
*
* @param	list	-	the list object.
*
* @return	A reference to the first element in the list container.
*/
extern void *cmn_list_front(cmn_list_t list);

/*
* Returns a direct reference to the last element in the list container.
*
* If the container is not empty, the function never fails.
* Otherwise, it causes undefined behavior.
*
* @param	list	-	the list object.
*
* @return	A reference to the last element in the list container.
*/
extern void *cmn_list_back(cmn_list_t list);

/*******************************************************************************
*                                   Iterators                                  *
*******************************************************************************/

/*
* Returns an iterator to the first element of the list container.
* If the list is empty, the returned iterator will be equal to cmn_list_end().
*
* This function never fails.
*
* @param	list	 -	the list object.
*
* @return	An iterator to the beginning of the list container.
*/
extern cmn_iterator_t cmn_list_begin(cmn_list_t list);

/*
* Returns an iterator to the element following the last element of the list.
* This element acts as a placeholder; attempting to access it results in
* undefined behavior.
*
* This function never fails.
*
* @param	list	 -	the list object.
*
* @return	An iterator to the element past the end of the list container.
*/
extern cmn_iterator_t cmn_list_end(cmn_list_t list);

/*******************************************************************************
*                                   Capacity                                   *
*******************************************************************************/

/*
* Returns whether the list container is empty.
*
* This function never fails.
*
* @param	list	 -	the list object.
*
* @return	true if the container size is 0, false otherwise.
*/
extern bool cmn_list_is_empty(cmn_list_t list);

/*
* Returns the number of elements in the list container.
*
* This function never fails.
*
* @param	list	-	the list object.
*
* @return	The number of elements in the container.
*/
extern size_t cmn_list_size(cmn_list_t list);

/*******************************************************************************
*                                   Modifiers                                  *
*******************************************************************************/

/*
* Removes all elements from the list container, leaving the container with a
* size of 0.
*
* All iterators related to this container are invalidated, except the end
* iterators.
*
* This function never fails.
*
* @param	list	-	the list object.
*
* @return	This function returns no value.
*/
extern void cmn_list_clear(cmn_list_t list);

/*
* Inserts an element before the specified position in the list.
*
* No iterators or references are invalidated.
*
* The behavior is undefined if the element is not in the list.
*
* @param	list	 -	the list object.
* @param	position -	iterator before which the content will be inserted.
* @param	value	 -	element value to insert.
* @param	inserted -  pointer where will be stored an iterator to the inserted
*                       element, or to the element that prevented the insertion.
*                       If NULL the iterator will not be returned.
* @return   On error, this function returns not zero.
*/
extern int cmn_list_insert(cmn_list_t list, cmn_iterator_t position, void *value, cmn_iterator_t *inserted);

/*
* Removes from the list container the element at the specified position.
*  The iterator position must be valid and dereferenceable. Thus, the end()
*  iterator (which is valid, but is not dereferenceable) cannot be used as a
*  value for position.
*
*  References and iterators to the erased elements are invalidated. Other
*  references and iterators are not affected.
*
*  If position is valid, the function never fails.
*  Otherwise, it causes undefined behavior.
*
* @param	list	 -	the list object.
* @param	position -	iterator to the element to remove.
* @param	iterator -  pointer where will be stored an iterator pointing to the
*                       element that followed the erased one. If NULL the 
*                       iterator will not be returned.
* @return   On error, this function returns not zero.
*/
extern int cmn_list_erase(cmn_list_t list, cmn_iterator_t position, cmn_iterator_t *iterator);

/*
* Prepends a new element to the beginning of the list.
*
* No iterators or references are invalidated.
*
* @param	list	-	the list object.
* @param	value	-	the value of the element to prepend.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_list_push_front(cmn_list_t list, void *value);

/*
* Appends the given element value to the end of the list.
*
* No iterators or references are invalidated.
*
* @param	list	-	the list object.
* @param	value	-	the value of the element to append.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_list_push_back(cmn_list_t list, void *value);

/*
* Removes the first element of the list.
*
* References and iterators to the erased element are invalidated. Other
* references and iterators are not affected.
*
* If the container is not empty, the function never fails.
* Otherwise, it causes undefined behavior.
*
* @param	list	-	the list object.
*
* @return	This function returns no value.
*/
extern void cmn_list_pop_front(cmn_list_t list);

/*
* Removes the last element of the list.
*
* References and iterators to the erased element are invalidated. Other
* references and iterators are not affected.
*
* If the container is not empty, the function never fails.
* Otherwise, it causes undefined behavior.
*
* @param	list	-	the list object.
*
* @return	This function returns no value.
*/
extern void cmn_list_pop_back(cmn_list_t list);

/*
* Resizes the list to contain n elements.
* If the current size is greater than n, the list is reduced to its first s
* elements.
* If the current size is less than s, additional copies of value are appended.
*
* References and iterators to the erased element are invalidated. Other
* references and iterators are not affected.
*
* @param	list	-	the list object.
* @param	s		-	new size of the list, expressed in number of elements.
* @param	value	-	the value to initialize the new elements with.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_list_resize(cmn_list_t list, size_t s, void *value);

/*
* Exchanges the contents of the list with those of other. Does not invoke any
* move, copy, or swap operations on individual elements.
*
* All iterators and references remain valid. It is unspecified whether an
* iterator holding the past-the-end value in this container will refer to this
* or the other container after the operation.
*
* The function never fails.
*
* @param	list	-	the list object.
* @param	other	-	the other list to exchange the contents with.
*
* @return	This function returns no value.
*/
extern void cmn_list_swap(cmn_list_t list, cmn_list_t other);

/*******************************************************************************
*                                  Operations                                  *
*******************************************************************************/

/*
* Merges two sorted lists into one. The lists should be sorted into ascending
* order.
* No elements are copied. The list other becomes empty after the operation.
* This operation is stable: for equivalent elements in the two lists, the
* elements from this shall always precede the elements from other, and the order
* of equivalent elements of this and other does not change.
*
* All iterators and references remain valid.
*
* If the comparison function is guaranteed to never fails, the function
* never fails.
*
* @param	list	-	the list object.
* @param	other	-	another list to merge.
* @param	comp	-	binary function which set a boolean as true if the first
*						argument is less than the second.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_list_merge(cmn_list_t list, cmn_list_t other, int (*comp)(void *a, void *b, bool *result));

/*
* Transfers the elements in the range [first, last) from another list into this.
* The element is inserted before the element pointed to by position.
* No elements are copied or moved, only the internal pointers of the list nodes
* are re-pointed.
* The behavior is undefined if position is an iterator in the range [first,last).
*
* If any of the iterators or ranges specified is not valid, or if position is
* in the range [first,last) it causes undefined behavior.
* Otherwise, the function never fails.
*
* @param	list		-	the list object.
* @param	other		-	another list to transfer the content from.
* @parma	position	-	element before which the content will be inserted.
* @param	first, last -	the range of elements to transfer from other to
							this.
*
* @return	This function returns no value.
*/
extern void cmn_list_splice(cmn_list_t list, cmn_list_t other, cmn_iterator_t position, cmn_iterator_t first, cmn_iterator_t last);

/*
* Removes from the container all elements that are equal to value.
*
* References and iterators to the erased elements are invalidated. Other
* references and iterators are not affected.
*
* @param	list	-	the list object.
* @param	value	-	value of the elements to be removed.
*
* @return	The number of elements removed.
*/
extern size_t cmn_list_remove(cmn_list_t list, void *value);

/*
* Removes all elements for which predicate pred returns true.
*
* References and iterators to the erased elements are invalidated. Other
* references and iterators are not affected.
*
* If the comparison function is guaranteed to never fails, the function
* never fails.
*
* @param	list	-	the list object.
* @param	pred	-	Unary predicate that, taking a value of the same type
*						as those contained in the forward_list object, set the
*						result as true for those values to be removed from the
*						container, and false otherwise.
* @param	removed	-	the pointer that will reference the number of elements
*						removed.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_list_remove_if(cmn_list_t list, int (*pred)(void *a, bool *result), size_t *removed);

/*
* Reverses the order of the elements in the list.
*
* No references or iterators become invalidated.
*
* The function never fails.
*
* @param	list	-	the list object.
*
* @return	This function returns no value.
*/
extern void cmn_list_reverse(cmn_list_t list);

/*
* Removes all consecutive duplicate elements from the list. Only the first
* element in each group of equal elements is left.
*
* References and iterators to the erased elements are invalidated. Other
* references and iterators are not affected.
*
* If the comparison function is guaranteed to never fails, the function
* never fails.
*
* @param	list	-	the list object.
* @param	comp	-	binary function which set a boolean as true if the first
*						argument is equal to the second.
* @param	removed	-	the pointer that will reference the number of elements
*						removed.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_list_unique(cmn_list_t list, int (*comp)(void *a, void *b, bool *result), size_t *removed);

/*
* Sorts the elements in ascending order. The order of equal elements is
* preserved.
* If an error occurs, the order of elements in the list is unspecified.
*
* No references or iterators become invalidated.
*
* @param	list	-	the list object.
* @param	comp	-	binary function which set a boolean as true if the first
*						argument is less than the second.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_list_sort(cmn_list_t list, int(*comp)(void *a, void *b, bool *result));
