#pragma once

#include "stack/linked_list_stack.h"

#include "list/linked_list.h"
#include "types/stack.h"

struct cmn_linked_list_stack {
    struct cmn_stack super;
    cmn_linked_list_t list;
};

extern int cmn_linked_list_stack_ctor(cmn_linked_list_stack_t stack);
