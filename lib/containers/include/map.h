#pragma once

#include <stddef.h>
#include <stdbool.h>

#include "iterator.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_map *cmn_map_t;

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
* @param	map	-	the map object.
*
* @return	This function returns no value.
*/
extern int cmn_map_destroy(cmn_map_t map);

/*******************************************************************************
*                                Element access                                *
*******************************************************************************/

/*
* Returns a reference to the mapped value of the element with key equivalent
* to key.
*
* If the element exists in the container, the function never fails.
* Otherwise, an ERANGE errno is returned.
*
* @param	map	-	the map object.
* @param	key		-	the key of the element to find.
* @param	value	-	the pointer that will reference the mapped value of
*						the requested element.
*
* @return	On success, this function returns zero.  On error, an errno ERANGE.
*/
extern int cmn_map_at(cmn_map_t map, void *key, void **value);

/*******************************************************************************
*                                   Iterators                                  *
*******************************************************************************/

/*
* Returns an iterator to the first element of the container.
* If the container is empty, the returned iterator will be equal to end().
*
* This function never fails.
*
* @param	map	 -	the map object.
*
* @return	An iterator to the beginning of the container.
*/
extern cmn_iterator_t cmn_map_begin(cmn_map_t map);

/*
* Returns an iterator to the element following the last element of the map.
* This element acts as a placeholder; attempting to access it results in
* undefined behavior.
*
* This function never fails.
*
* @param	map	 -	the map object.
*
* @return	An iterator to the element past the end of the container.
*/
extern cmn_iterator_t cmn_map_end(cmn_map_t map);

/*******************************************************************************
*                                   Capacity                                   *
*******************************************************************************/

/*
* Returns whether the container is empty.
*
* @param	map	 -	the map object.
*
* @return	true if the container empty, false otherwise.
*/
extern bool cmn_map_is_empty(cmn_map_t map);

/*
* Returns the number of elements in the container.
*
* This function never fails.
*
* @param	map	-	the map object.
*
* @return	The number of elements in the container.
*/
extern size_t cmn_map_size(cmn_map_t map);

/*******************************************************************************
*                                   Modifiers                                  *
*******************************************************************************/

/*
* Replace the default comparer function with a cutom one.
*
* This function never fails
*
* @param	map	-	the map object.
* @param    comp -  the comparator function.
*
* @return	This function returns no value.
*/
extern void cmn_map_set_key_comparer(cmn_map_t map, int (*comp)(void *a, void *b, bool *result));

/*
* Removes all elements from the container, leaving the container with a size
* of 0.
*
* All iterators related to this container are invalidated, except the end
* iterators.
*
* This function never fails.
*
* @param	map	-	the map object.
*
* @return	This function returns no value.
*/
extern void cmn_map_clear(cmn_map_t map);

/*
* Inserts an element into the container, if the container doesn't already
* contain an element with an equivalent key.
*
* No iterators or references are invalidated.
*
* The behavior is undefined if the element is not in the list.
*
* @param	map	-	the map object.
* @param	key		-	the key of the element to insert.
* @param	value	-	element value to insert.
* @param	inserted -  pointer where will be stored an iterator to the inserted
*                       element, or to the element that prevented the insertion.
*                       If NULL the iterator will not be returned.
* @return   On error, this function returns not zero.
*/
extern int cmn_map_insert(cmn_map_t map, void *key, void *value, cmn_iterator_t *inserted);

/*
* Inserts an element into the container, if a key equivalent to key already 
* exists in the container, assigns value to the mapped_type corresponding to the 
* key. If the key does not exist, inserts the new value as if by insert.
*
* No iterators or references are invalidated.
*
* The behavior is undefined if the element is not in the list.
*
* @param	map	-	the map object.
* @param	key		-	the key used both to look up and to insert if not found.
* @param	value	-	element value to insert.
* @param	inserted -  pointer where will be stored an iterator to the inserted.
*                       If NULL the iterator will not be returned.
* @return   On error, this function returns not zero.
*/
extern int cmn_map_insert_or_assign(cmn_map_t map, void *key, void *value, cmn_iterator_t *inserted);

/*
* Removes from the container the element at the specified position.
* The iterator position must be valid and dereferenceable. Thus, the end()
* iterator (which is valid, but is not dereferenceable) cannot be used as a
* value for position.
*
* References and iterators to the erased elements are invalidated. Other
* references and iterators are not affected.
*
* If position is valid, the function never fails.
* Otherwise, it causes undefined behavior.
*
* @param	map	 -	the map object.
* @param	position -	iterator to the element to remove.
* @param	iterator - An iterator pointing to the element that followed the erased one.
* @param	iterator -  pointer where will be stored an iterator pointing to the
*                       element that followed the erased one. If NULL the 
*                       iterator will not be returned.
* @return   On error, this function returns not zero.
*/
extern int cmn_map_erase(cmn_map_t map, cmn_iterator_t position, cmn_iterator_t *iterator);

/*
* Exchanges the contents of the container  with those of other. Does not invoke
* any move, copy, or swap operations on individual elements.
*
* All iterators and references remain valid. It is unspecified whether an
* iterator holding the past-the-end value in this container will refer to this
* or the other container after the operation.
*
* The function never fails.
*
* @param	map	-	the map object.
* @param	other	-	the other list to exchange the contents with.
*
* @return	This function returns no value.
*/
extern void cmn_map_swap(cmn_map_t map, cmn_map_t other);

/*******************************************************************************
*                                   Lookup                                     *
*******************************************************************************/

/*
* Returns the number of elements with key that compares equivalent to the 
* specified argument, which is either 1 or 0 since this container does not 
* allow duplicates.
* 
* @param	map	-	the map object.
* @param	key		-	key value of the elements to count.
* @param	comp	-	binary function which set a boolean as true if the first
*						argument is equal to the second.
* @param	count	-	the pointer that will reference the number of elements 
*						with key that compares equivalent to key, which is 
*						either 1 or 0.
* 
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_map_count(cmn_map_t map, void *key, size_t *count);

/*
* Finds an element with key equivalent to key.
*
* @param	map	 -	the map object.
* @param	key		 -	key value of the element to search for.
* @param	comp	 -	binary function which set a boolean as true if the first
*						argument is equal to the second.
* @param	iterator -	the pointer that will reference the iterator to an 
*						element with key equivalent to key. If no such element 
*						is found, the end iterator is returned.
*
* @return	On success, this function returns zero.  On error, an errno [...].
*/
extern int cmn_map_find(cmn_map_t map, void *key, cmn_iterator_t *iterator);

/*
* Checks if there is an element with key equivalent to key.
*
* @param	map	        -	the map object.
* @param	key         -	key value of the element to search for.
*
* @return	true if there is such an element, otherwise false.
*/
extern bool cmn_map_contains(cmn_map_t map, void *key);
