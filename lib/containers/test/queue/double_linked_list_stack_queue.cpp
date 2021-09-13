#include <gtest/gtest.h>

extern "C" {
#include "queue.h"
#include "queue/double_linked_list_stack_queue.h"
}

TEST(cmn_double_linked_list_stack_queue, empty_after_initialization) {
    bool is_empty;
    auto queue = (cmn_queue_t) cmn_double_linked_list_stack_queue_init();
    is_empty = cmn_queue_is_empty(queue);
    cmn_queue_destroy(queue);
    ASSERT_EQ(is_empty, true);
}

TEST(cmn_double_linked_list_stack_queue, non_empty_after_single_insertion) {
    bool is_empty;
    auto queue = (cmn_queue_t) cmn_double_linked_list_stack_queue_init();
    cmn_queue_enqueue(queue, (void *) 1);
    is_empty = cmn_queue_is_empty(queue);
    cmn_queue_destroy(queue);
    ASSERT_EQ(is_empty, false);
}

TEST(cmn_double_linked_list_stack_queue, empty_after_single_deletion_after_single_insertion) {
    bool is_empty;
    auto queue = (cmn_queue_t) cmn_double_linked_list_stack_queue_init();
    cmn_queue_enqueue(queue, (void *) 1);
    cmn_queue_dequeue(queue);
    is_empty = cmn_queue_is_empty(queue);
    cmn_queue_destroy(queue);
    ASSERT_EQ(is_empty, true);
}

TEST(cmn_double_linked_list_stack_queue, item_inserted_is_retrievable) {
    void *inserted = (void *) 1;
    void *retrieved;
    auto queue = (cmn_queue_t) cmn_double_linked_list_stack_queue_init();
    cmn_queue_enqueue(queue, inserted);
    retrieved = cmn_queue_dequeue(queue);
    cmn_queue_destroy(queue);
    ASSERT_EQ(inserted, retrieved);
}

TEST(cmn_double_linked_list_stack_queue, is_FIFO) {
    void *e1 = (void *) 1;
    void *e2 = (void *) 2;
    void *e3 = (void *) 3;
    bool isFIFO = true;
    auto queue = (cmn_queue_t) cmn_double_linked_list_stack_queue_init();
    cmn_queue_enqueue(queue, e1);
    cmn_queue_enqueue(queue, e2);
    cmn_queue_enqueue(queue, e3);
    isFIFO &= (cmn_queue_dequeue(queue) == e1);
    isFIFO &= (cmn_queue_dequeue(queue) == e2);
    isFIFO &= (cmn_queue_dequeue(queue) == e3);
    cmn_queue_destroy(queue);
    ASSERT_EQ(isFIFO, true);
}
