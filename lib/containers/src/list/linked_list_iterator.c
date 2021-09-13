#include "linked_list_iterator.h"

#include <stdlib.h>
#include <string.h>

static struct cmn_iterator_vtbl *get_iterator_vtbl();

int destroy(struct cmn_iterator *iterator);

static void *data(struct cmn_iterator *iterator);

static struct cmn_iterator *next(struct cmn_iterator *iterator);

static struct cmn_iterator *prev(struct cmn_iterator *iterator);

static bool begin(struct cmn_iterator *iterator);

static bool end(struct cmn_iterator *iterator);

extern struct linked_list_iterator *
linked_list_iterator_init(struct cmn_linked_list *list, struct list_element *element) {
    struct linked_list_iterator *iterator;
    iterator = malloc(sizeof *iterator);
    if (iterator) {
        iterator->super.__ops_vptr = get_iterator_vtbl();
        iterator->list = list;
        iterator->element = element;
    }
    return iterator;
}

static struct cmn_iterator_vtbl *get_iterator_vtbl() {
    struct cmn_iterator_vtbl vtbl_zero = {0};
    if (!memcmp(&vtbl_zero, &__cmn_iterator_ops_vtbl, sizeof vtbl_zero)) {
        __cmn_iterator_ops_vtbl.destroy = destroy;
        __cmn_iterator_ops_vtbl.data = data;
        __cmn_iterator_ops_vtbl.next = next;
        __cmn_iterator_ops_vtbl.prev = prev;
        __cmn_iterator_ops_vtbl.begin = begin;
        __cmn_iterator_ops_vtbl.end = end;
    }
    return &__cmn_iterator_ops_vtbl;
}

int destroy(struct cmn_iterator *iterator) {
    return 0;
}

static void *data(struct cmn_iterator *iterator) {
    struct linked_list_iterator *this = (struct linked_list_iterator *) iterator;
    return this->element->data;
}

static struct cmn_iterator *next(struct cmn_iterator *iterator) {
    struct linked_list_iterator *this = (struct linked_list_iterator *) iterator;
    this->element = this->element->next;
    return iterator;
}

static struct cmn_iterator *prev(struct cmn_iterator *iterator) {
    struct linked_list_iterator *this = (struct linked_list_iterator *) iterator;
    this->element = get_previous_element(this->list, this->element);
    return iterator;
}

static bool begin(struct cmn_iterator *iterator) {
    struct linked_list_iterator *this = (struct linked_list_iterator *) iterator;
    return this->element == this->list->head;
}

static bool end(struct cmn_iterator *iterator) {
    struct linked_list_iterator *this = (struct linked_list_iterator *) iterator;
    return this->element == NULL;
}
