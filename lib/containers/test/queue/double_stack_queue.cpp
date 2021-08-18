#include <gtest/gtest.h>

extern "C" {
#include "queue.h"
#include "queue/double_stack_queue.h"
}

TEST(cmn_double_stack_queue, empty_after_initialization) {
	bool is_empty;
	struct cmn_double_stack_queue queue;
	cmn_double_stack_queue_init(&queue);
	is_empty = cmn_queue_is_empty(&(queue.super));
	cmn_queue_destroy(&(queue.super));
	ASSERT_EQ(is_empty, true);
}

TEST(cmn_double_stack_queue, non_empty_after_single_insertion) {
	bool is_empty;
	struct cmn_double_stack_queue queue;
	cmn_double_stack_queue_init(&queue);
	cmn_queue_enqueue(&(queue.super), (void*)1);
	is_empty = cmn_queue_is_empty(&(queue.super));
	cmn_queue_destroy(&(queue.super));
	ASSERT_EQ(is_empty, false);
}

TEST(cmn_double_stack_queue, empty_after_single_deletion_after_single_insertion) {
	bool is_empty;
	struct cmn_double_stack_queue queue;
	cmn_double_stack_queue_init(&queue);
	cmn_queue_enqueue(&(queue.super), (void*)1);
	cmn_queue_dequeue(&(queue.super));
	is_empty = cmn_queue_is_empty(&(queue.super));
	cmn_queue_destroy(&(queue.super));
	ASSERT_EQ(is_empty, true);
}

TEST(cmn_double_stack_queue, item_inserted_is_retrievable) {
	void* inserted = (void*)1;
	void* retrieved = NULL;
	struct cmn_double_stack_queue queue;
	cmn_double_stack_queue_init(&queue);
	cmn_queue_enqueue(&(queue.super), inserted);
	retrieved = cmn_queue_dequeue(&(queue.super));
	cmn_queue_destroy(&(queue.super));
	ASSERT_EQ(inserted, retrieved);
}

TEST(cmn_double_stack_queue, is_FIFO) {
	void* e1 = (void*)1;
	void* e2 = (void*)2;
	void* e3 = (void*)3;
	bool isFIFO = true;
	struct cmn_double_stack_queue queue;
	cmn_double_stack_queue_init(&queue);
	cmn_queue_enqueue(&(queue.super), e1);
	cmn_queue_enqueue(&(queue.super), e2);
	cmn_queue_enqueue(&(queue.super), e3);
	isFIFO &= (cmn_queue_dequeue(&(queue.super)) == e1);
	isFIFO &= (cmn_queue_dequeue(&(queue.super)) == e2);
	isFIFO &= (cmn_queue_dequeue(&(queue.super)) == e3);
	cmn_queue_destroy(&(queue.super));
	ASSERT_EQ(isFIFO, true);
}
