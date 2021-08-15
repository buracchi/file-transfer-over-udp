#include "iterator.h"

/*******************************************************************************
*                                  Operations                                  *
*******************************************************************************/

extern struct cmn_iterator* cmn_iterator_advance(struct cmn_iterator* iterator, int n) {
	for (int i = n; i != 0; n ? i++ : i--) {
		n ? iterator->__ops_vptr->next(iterator) : iterator->__ops_vptr->prev(iterator);
	}
	return iterator;
}

extern size_t cmn_iterator_distance(struct cmn_iterator* first, struct cmn_iterator* last) {
	size_t dist = 0;
	while (first != last) {
		first->__ops_vptr->next(first);
		dist++;
	}
	for (size_t i = 0; i < dist; i++) {
		first->__ops_vptr->prev(first);		//Not very smart duh
	}
	return dist;
}
