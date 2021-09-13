#include <gtest/gtest.h>

extern "C" {
#include "list.h"
#include "list/linked_list.h"
}

TEST(cmn_linked_list, empty_after_initialization) {
    bool is_empty;
    auto list = (cmn_list_t) cmn_linked_list_init();
    is_empty = cmn_list_is_empty(list);
    cmn_list_destroy(list);
    ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list, non_empty_after_single_insertion) {
    bool is_empty;
    auto list = (cmn_list_t) cmn_linked_list_init();
    cmn_list_push_back(list, (void *) 1);
    is_empty = cmn_list_is_empty(list);
    cmn_list_destroy(list);
    ASSERT_EQ(is_empty, false);
}

TEST(cmn_linked_list, empty_after_single_deletion_after_single_insertion) {
    bool is_empty;
    auto list = (cmn_list_t) cmn_linked_list_init();
    cmn_list_push_back(list, (void *) 1);
    cmn_list_pop_back(list);
    is_empty = cmn_list_is_empty(list);
    cmn_list_destroy(list);
    ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list, item_inserted_is_retrievable) {
    void *inserted = (void *) 1;
    void *retrieved;
    auto list = (cmn_list_t) cmn_linked_list_init();
    cmn_list_push_back(list, inserted);
    retrieved = cmn_list_front(list);
    cmn_list_destroy(list);
    ASSERT_EQ(inserted, retrieved);
}
