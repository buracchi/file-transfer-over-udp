#pragma once

#include "list.h"
#include "iterator.h"

#include "types/list/linked_list.h"
#include "linked_list_element.h"

extern struct linked_list_iterator *
linked_list_iterator_init(struct cmn_linked_list *list, struct list_element *element);
