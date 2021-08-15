#pragma once

#include "list.h"
#include "iterator.h"

#include "linked_list_element.h"

struct linked_list_iterator {
    struct cmn_iterator super;
	struct cmn_linked_list* list;
	struct list_element* element;
};

extern struct linked_list_iterator* linked_list_iterator_init(struct cmn_linked_list* list, struct list_element* element);
