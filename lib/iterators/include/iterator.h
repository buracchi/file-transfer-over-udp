#pragma once

#include <stddef.h>
#include <stdbool.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_iterator {
	struct cmn_iterator_vtbl* __ops_vptr;
};

static struct cmn_iterator_vtbl {
	// Member functions
	int		(*destroy)	(struct cmn_iterator* iterator);
	// Element access
	void*	(*data)		(struct cmn_iterator* iterator);
	struct cmn_iterator*	(*next)	(struct cmn_iterator* iterator);
	struct cmn_iterator*	(*prev)	(struct cmn_iterator* iterator);
	bool	(*begin)	(struct cmn_iterator* iterator);
	bool	(*end)		(struct cmn_iterator* iterator);
} __cmn_iterator_ops_vtbl __attribute__((unused)) = { 0, 0, 0, 0, 0, 0 };

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

/*
* Destroys the iterator object.
*
* This function never fails.
*
* @param	iterator	-	the iterator object.
*
* @return	This function returns no value.
*/
static inline int cmn_iterator_destroy(struct cmn_iterator* iterator) {
	return iterator->__ops_vptr->destroy(iterator);
}

/*******************************************************************************
*                                Element access                                *
*******************************************************************************/

/*
* Returns a pointer to the block of memory containing the elements of the
* container.
*
* If the iterator points to a dereferenceable container element this function
* never fails. Otherwise, it causes undefined behavior.
*
* @param	iterator	-	the iterator object.
* @param	data		-	the pointer that will reference the data.
*
* @return	A reference to the data  associated to the container element
*			pointed by the iterator.
*/
static inline void* cmn_iterator_data(struct cmn_iterator* iterator) {
	return iterator->__ops_vptr->data(iterator);
}

/*
* Increments iterator this by 1 element.
*
* If the iterator doesn't points to an end container element this function
* never fails. Otherwise, it causes undefined behavior.
*
* @param	iterator	-	the iterator object.
*
* @return	A reference to this iterator.
*/
static inline struct cmn_iterator* cmn_iterator_next(struct cmn_iterator* iterator) {
	return iterator->__ops_vptr->next(iterator);
}

/*
* Decrements iterator this by 1 element.
*
* If the iterator doesn't points to a begin container element this function
* never fails. Otherwise, it causes undefined behavior.
*
* @param	iterator	-	the iterator object.
*
* @return	A reference to this iterator.
*/
static inline struct cmn_iterator* cmn_iterator_prev(struct cmn_iterator* iterator) {
	return iterator->__ops_vptr->prev(iterator);
}

/*
* Returns true if the iterator is followed by an element.
*
* This function never fails.
*
* @param	iterator	-	the iterator object.
* @param	has_next	-	the pointer that will reference the result.
*
* @return	true if the iterator is followed by an element, false otherwise.
*/
static inline bool cmn_iterator_begin(struct cmn_iterator* iterator) {
	return iterator->__ops_vptr->begin(iterator);
}

/*
* Returns true if the iterator is preceded by an element.
*
* This function never fails.
*
* @param	iterator	-	the iterator object.
* @param	has_prev 	-	the pointer that will reference the result.
*
* @return	true if the iterator is preceded by an element, false otherwise.
*/
static inline bool cmn_iterator_end(struct cmn_iterator* iterator) {
	return iterator->__ops_vptr->end(iterator);
}

/*******************************************************************************
*                                  Operations                                  *
*******************************************************************************/

/*
* Increments iterator this by n elements.
* If n is negative, the iterator is decremented.
*
* This function never fails.
*
* @param	iterator	-	the iterator object.
* @param	n			-	number of elements the iterator should be advanced.
*
* @return	A reference to this iterator.
*/
extern struct cmn_iterator* cmn_iterator_advance(struct cmn_iterator* iterator, int n);

/*
* Returns the number of hops from first to last.
* 
* If last is reachable from first this function never fails.
* Otherwise, it causes undefined behavior.
*
* @param	first	 -	iterator pointing to the first element.
* @param	last	 -	iterator pointing to the end of the range.
*
* @return	The number of elements between first and last.
*/
extern size_t cmn_iterator_distance(struct cmn_iterator* first, struct cmn_iterator* last);
