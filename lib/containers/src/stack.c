#include "stack.h"
#include "types/stack.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

extern inline int cmn_stack_destroy(cmn_stack_t stack) {
    int ret = stack->__ops_vptr->destroy(stack);
    if (!ret) {
        free(stack);
    }
    return ret;
}

extern inline void *cmn_stack_peek(cmn_stack_t stack) {
    return stack->__ops_vptr->peek(stack);
}

extern inline bool cmn_stack_is_empty(cmn_stack_t stack) {
    return stack->__ops_vptr->is_empty(stack);
}

extern inline size_t cmn_stack_get_size(cmn_stack_t stack) {
    return stack->__ops_vptr->get_size(stack);
}

extern inline int cmn_stack_push(cmn_stack_t stack, void *item) {
    return stack->__ops_vptr->push(stack, item);
}

extern inline void *cmn_stack_pop(cmn_stack_t stack) {
    return stack->__ops_vptr->pop(stack);
}
