#include "stack.h"
#include "stack/linked_list_stack.h"

#include <stdlib.h>
#include <string.h>

#include "list.h"
#include "types/stack.h"
#include "types/stack/linked_list_stack.h"
#include "try.h"

static struct cmn_stack_vtbl *get_stack_vtbl();

static int destroy(cmn_stack_t stack);

static void *peek(cmn_stack_t stack);

static bool is_empty(cmn_stack_t stack);

static size_t get_size(cmn_stack_t stack);

static int push(cmn_stack_t stack, void *item);

static void *pop(cmn_stack_t stack);

extern cmn_linked_list_stack_t cmn_linked_list_stack_init() {
    cmn_linked_list_stack_t stack;
    try(stack = malloc(sizeof *stack), NULL, fail);
    try(cmn_linked_list_stack_ctor(stack), 1, fail2);
    return stack;
fail2:
    free(stack);
fail:
    return NULL;
}

extern int cmn_linked_list_stack_ctor(cmn_linked_list_stack_t stack) {
    memset(stack, 0, sizeof *stack);
    stack->super.__ops_vptr = get_stack_vtbl();
    try(stack->list = cmn_linked_list_init(), NULL, fail);
    return 0;
fail:
    return 1;
}

static struct cmn_stack_vtbl *get_stack_vtbl() {
    struct cmn_stack_vtbl vtbl_zero = {0};
    if (!memcmp(&vtbl_zero, &__cmn_stack_ops_vtbl, sizeof vtbl_zero)) {
        __cmn_stack_ops_vtbl.destroy = destroy;
        __cmn_stack_ops_vtbl.peek = peek;
        __cmn_stack_ops_vtbl.is_empty = is_empty;
        __cmn_stack_ops_vtbl.get_size = get_size;
        __cmn_stack_ops_vtbl.push = push;
        __cmn_stack_ops_vtbl.pop = pop;
    }
    return &__cmn_stack_ops_vtbl;
}

static int destroy(cmn_stack_t stack) {
    cmn_linked_list_stack_t this = (cmn_linked_list_stack_t) stack;
    return cmn_list_destroy((cmn_list_t) this->list);
}

static void *peek(cmn_stack_t stack) {
    cmn_linked_list_stack_t this = (cmn_linked_list_stack_t) stack;
    return cmn_list_front((cmn_list_t) this->list);
}

static bool is_empty(cmn_stack_t stack) {
    cmn_linked_list_stack_t this = (cmn_linked_list_stack_t) stack;
    return cmn_list_is_empty((cmn_list_t) this->list);
}

static size_t get_size(cmn_stack_t stack) {
    cmn_linked_list_stack_t this = (cmn_linked_list_stack_t) stack;
    return cmn_list_size((cmn_list_t) this->list);
}

static int push(cmn_stack_t stack, void *item) {
    cmn_linked_list_stack_t this = (cmn_linked_list_stack_t) stack;
    return cmn_list_push_front((cmn_list_t) this->list, item);
}

static void *pop(cmn_stack_t stack) {
    cmn_linked_list_stack_t this = (cmn_linked_list_stack_t) stack;
    void *popped;
    popped = cmn_list_front((cmn_list_t) this->list);
    cmn_list_pop_front((cmn_list_t) this->list);
    return popped;
}
