#include <gtest/gtest.h>

extern "C" {
#include "stack.h"
#include "stack/linked_list_stack.h"
}

TEST(cmn_linked_list_stack, empty_after_initialization) {
    bool is_empty;
    auto stack = (cmn_stack_t) cmn_linked_list_stack_init();
    is_empty = cmn_stack_is_empty(stack);
    cmn_stack_destroy(stack);
    ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list_stack, non_empty_after_single_insertion) {
    bool is_empty;
    auto stack = (cmn_stack_t) cmn_linked_list_stack_init();
    cmn_stack_push(stack, (void *) 1);
    is_empty = cmn_stack_is_empty(stack);
    cmn_stack_destroy(stack);
    ASSERT_EQ(is_empty, false);
}

TEST(cmn_linked_list_stack, empty_after_single_deletion_after_single_insertion) {
    bool is_empty;
    auto stack = (cmn_stack_t) cmn_linked_list_stack_init();
    cmn_stack_push(stack, (void *) 1);
    cmn_stack_pop(stack);
    is_empty = cmn_stack_is_empty(stack);
    cmn_stack_destroy(stack);
    ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list_stack, item_inserted_is_retrievable) {
    void *inserted = (void *) 1;
    void *retrieved;
    auto stack = (cmn_stack_t) cmn_linked_list_stack_init();
    cmn_stack_push(stack, inserted);
    retrieved = cmn_stack_pop(stack);
    cmn_stack_destroy(stack);
    ASSERT_EQ(inserted, retrieved);
}

TEST(cmn_linked_list_stack, is_LIFO) {
    void *e1 = (void *) 1;
    void *e2 = (void *) 2;
    void *e3 = (void *) 3;
    bool isLIFO = true;
    auto stack = (cmn_stack_t) cmn_linked_list_stack_init();
    cmn_stack_push(stack, e1);
    cmn_stack_push(stack, e2);
    cmn_stack_push(stack, e3);
    isLIFO &= (cmn_stack_pop(stack) == e3);
    isLIFO &= (cmn_stack_pop(stack) == e2);
    isLIFO &= (cmn_stack_pop(stack) == e1);
    cmn_stack_destroy(stack);
    ASSERT_EQ(isLIFO, true);
}
