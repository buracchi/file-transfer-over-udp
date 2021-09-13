#include "linked_list_element.h"

#include <stdlib.h>

extern inline struct list_element *list_element_init(void *data, struct list_element *next) {
    struct list_element *element;
    element = malloc(sizeof *element);
    if (element) {
        element->data = data;
        element->next = next;
    }
    return element;
}

extern inline void element_destroy(struct list_element *element) {
    free(element);
}

extern inline struct list_element *get_previous_element(struct cmn_linked_list *list, struct list_element *element) {
    struct list_element *e;
    e = list->head;
    while (e->next != element) {
        e = e->next;
    }
    return e;
}
