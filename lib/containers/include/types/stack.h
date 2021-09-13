#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct cmn_stack {
    struct cmn_stack_vtbl *__ops_vptr;
} *cmn_stack_t;

static struct cmn_stack_vtbl {
    int (*destroy)(cmn_stack_t stack);

    void *(*peek)(cmn_stack_t stack);

    bool (*is_empty)(cmn_stack_t stack);

    size_t (*get_size)(cmn_stack_t stack);

    int (*push)(cmn_stack_t stack, void *item);

    void *(*pop)(cmn_stack_t stack);
} __cmn_stack_ops_vtbl = {0, 0, 0, 0, 0, 0};
