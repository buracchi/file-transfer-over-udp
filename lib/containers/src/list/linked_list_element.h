#pragma once

#include "list/linked_list.h"
#include "types/list/linked_list.h"

extern struct list_element *list_element_init(void *data, struct list_element *next);

extern void element_destroy(struct list_element *element);

extern struct list_element *get_previous_element(struct cmn_linked_list *list, struct list_element *element);
