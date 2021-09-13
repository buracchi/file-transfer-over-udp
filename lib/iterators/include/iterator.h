#pragma once

#include <stddef.h>
#include <stdbool.h>

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

typedef struct cmn_iterator *cmn_iterator_t;

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
extern int cmn_iterator_destroy(cmn_iterator_t iterator);

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
extern void *cmn_iterator_data(cmn_iterator_t iterator);

/*
* Increments iterator this by 1 element.
*
* If the iterator doesn't point to an end container element this function
* never fails. Otherwise, it causes undefined behavior.
*
* @param	iterator	-	the iterator object.
*
* @return	A reference to this iterator.
*/
extern cmn_iterator_t cmn_iterator_next(cmn_iterator_t iterator);

/*
* Decrements iterator this by 1 element.
*
* If the iterator doesn't point to a beginning container element this function
* never fails. Otherwise, it causes undefined behavior.
*
* @param	iterator	-	the iterator object.
*
* @return	A reference to this iterator.
*/
extern cmn_iterator_t cmn_iterator_prev(cmn_iterator_t iterator);

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
extern bool cmn_iterator_begin(cmn_iterator_t iterator);

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
extern bool cmn_iterator_end(cmn_iterator_t iterator);

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
extern cmn_iterator_t cmn_iterator_advance(cmn_iterator_t iterator, int n);

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
extern size_t cmn_iterator_distance(cmn_iterator_t first, cmn_iterator_t last);
