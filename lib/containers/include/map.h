#pragma once

#include <stdbool.h>

#include "iterator.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_map {
    struct cmn_map_vtbl* __ops_vptr;
};

static struct cmn_map_vtbl {
	// Member functions
	int		(*destroy)		(struct cmn_map* map);
	// Element access
	int		(*at)			(struct cmn_map* map, void* key, void** value);
	int		(*at2)			(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), void** value);
	// Iterators
	struct cmn_iterator*	(*begin)			(struct cmn_map* map);
	struct cmn_iterator*	(*end)				(struct cmn_map* map);
	// Capacity
	bool	(*is_empty)		(struct cmn_map* map);
	size_t	(*size)			(struct cmn_map* map);
	// Modifiers
	void	(*clear)		(struct cmn_map* map);
	struct cmn_iterator*	(*insert)			(struct cmn_map* map, void* key, void* value);
	struct cmn_iterator*	(*insert_or_assign)	(struct cmn_map* map, void* key, void* value);
	struct cmn_iterator*	(*erase)			(struct cmn_map* map, struct cmn_iterator* position);
	void	(*swap)			(struct cmn_map* map, struct cmn_map* other);
	// <T> (_*extract)(struct cmn_map* map, ...)
	// Lookup
	int		(*count)		(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), size_t* count);
	int		(*find)			(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), struct cmn_iterator** iterator);
} __cmn_map_ops_vtbl __attribute__((unused)) = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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
static inline int cmn_map_destroy(struct cmn_map* map) {
	return map->__ops_vptr->destroy(map);
}

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
* @param	value	-	the pointer that will reference the the mapped value of
*						the requested element.
*
* @return	On success, this function returns zero.  On error, an errno ERANGE.
*/
static inline int cmn_map_at(struct cmn_map* map, void* key, void** value) {
	return map->__ops_vptr->at(map, key, value);
}

/**
 * @brief 
 * 
 * @param map 
 * @param key 
 * @param comp 
 * @param value 
 * @return  
 */
static inline int cmn_map_at2(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), void** value) {
	return map->__ops_vptr->at2(map, key, comp, value);
}

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
static inline struct cmn_iterator* cmn_map_begin(struct cmn_map* map) {
	return map->__ops_vptr->begin(map);
}

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
static inline struct cmn_iterator* cmn_map_end(struct cmn_map* map) {
	return map->__ops_vptr->end(map);
}

/*******************************************************************************
*                                   Capacity                                   *
*******************************************************************************/

/*
* Returns whether the container is empty.
*
* This function never fails.
*
* @param	map	 -	the map object.
*
* @return	true if the container empty, false otherwise.
*/
static inline bool cmn_map_is_empty(struct cmn_map* map) {
	return map->__ops_vptr->is_empty(map);
}

/*
* Returns the number of elements in the container.
*
* This function never fails.
*
* @param	map	-	the map object.
*
* @return	The number of elements in the container.
*/
static inline size_t cmn_map_size(struct cmn_map* map) {
	return map->__ops_vptr->size(map);
}

/*******************************************************************************
*                                   Modifiers                                  *
*******************************************************************************/

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
static inline void cmn_map_clear(struct cmn_map* map) {
	map->__ops_vptr->clear(map);
}

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
*
* @return	returns an iterator to the inserted element, or to the element that 
*			prevented the insertion. On error, this function returns NULL.
*/
static inline struct cmn_iterator* cmn_map_insert(struct cmn_map* map, void* key, void* value) {
	return map->__ops_vptr->insert(map, key, value);
}

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
*
* @return	returns an iterator to the inserted element. On error, this function 
*			returns NULL.
*/
static inline struct cmn_iterator* cmn_map_insert_or_assign(struct cmn_map* map, void* key, void* value) {
	return map->__ops_vptr->insert_or_assign(map, key, value);
}

/*
* Removes from the container the element at the specified position.
* The iterator position must be valid and dereferenceable. Thus the end()
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
*
* @return	An iterator pointing to the element that followed the erased one.
*/
static inline struct cmn_iterator* cmn_map_erase(struct cmn_map* map, struct cmn_iterator* position) {
	return map->__ops_vptr->erase(map, position);
}

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
static inline void cmn_map_swap(struct cmn_map* map, struct cmn_map* other) {
	map->__ops_vptr->swap(map, other);
}

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
static inline int cmn_map_count(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), size_t* count) {
	return map->__ops_vptr->count(map, key, comp, count);
}

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
static inline int cmn_map_find(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), struct cmn_iterator** iterator) {
	return map->__ops_vptr->find(map, key, comp, iterator);
}
