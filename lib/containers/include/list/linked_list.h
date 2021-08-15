/**
 * @brief This object inherit from the list object and is indeed polimorphyc to
 *  it, you can use all the function defined in list.h casting a
 *  a cmn_linked_list object.
 */
#pragma once

#include "list.h"

/*******************************************************************************
*                                 Member types                                 *
*******************************************************************************/

struct cmn_linked_list {
	struct cmn_list super;
	size_t size;
	void* head;
	void* tail;
};

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

extern int cmn_linked_list_init(struct cmn_linked_list* list);
