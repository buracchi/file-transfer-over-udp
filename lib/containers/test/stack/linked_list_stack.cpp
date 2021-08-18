#include <gtest/gtest.h>

extern "C" {
#include "stack.h"
#include "stack/linked_list_stack.h"
}

TEST(cmn_linked_list_stack, empty_after_initialization) {
	bool is_empty;
	struct cmn_linked_list_stack stack;
	cmn_linked_list_stack_init(&stack);
	is_empty = cmn_stack_is_empty(&(stack.super));
	cmn_stack_destroy(&(stack.super));
	ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list_stack, non_empty_after_single_insertion) {
	bool is_empty;
	struct cmn_linked_list_stack stack;
	cmn_linked_list_stack_init(&stack);
	cmn_stack_push(&(stack.super), (void*)1);
	is_empty = cmn_stack_is_empty(&(stack.super));
	cmn_stack_destroy(&(stack.super));
	ASSERT_EQ(is_empty, false);
}

TEST(cmn_linked_list_stack, empty_after_single_deletion_after_single_insertion) {
	bool is_empty;
	struct cmn_linked_list_stack stack;
	cmn_linked_list_stack_init(&stack);
	cmn_stack_push(&(stack.super), (void*)1);
	cmn_stack_pop(&(stack.super));
	is_empty = cmn_stack_is_empty(&(stack.super));
	cmn_stack_destroy(&(stack.super));
	ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list_stack, item_inserted_is_retrievable) {
	void* inserted = (void*)1;
	void* retrieved = NULL;
	struct cmn_linked_list_stack stack;
	cmn_linked_list_stack_init(&stack);
	cmn_stack_push(&(stack.super), inserted);
	retrieved = cmn_stack_pop(&(stack.super));
	cmn_stack_destroy(&(stack.super));
	ASSERT_EQ(inserted, retrieved);
}

TEST(cmn_linked_list_stack, is_LIFO) {
	void* e1 = (void*)1;
	void* e2 = (void*)2;
	void* e3 = (void*)3;
	bool isLIFO = true;
	struct cmn_linked_list_stack stack;
	cmn_linked_list_stack_init(&stack);
	cmn_stack_push(&(stack.super), e1);
	cmn_stack_push(&(stack.super), e2);
	cmn_stack_push(&(stack.super), e3);
	isLIFO &= (cmn_stack_pop(&(stack.super)) == e3);
	isLIFO &= (cmn_stack_pop(&(stack.super)) == e2);
	isLIFO &= (cmn_stack_pop(&(stack.super)) == e1);
	cmn_stack_destroy(&(stack.super));
	ASSERT_EQ(isLIFO, true);
}
