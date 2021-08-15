#include <gtest/gtest.h>

extern "C" {
#include "list.h"
#include "list/linked_list.h"
}

TEST(cmn_linked_list, empty_after_initialization) {
	struct cmn_linked_list list;
	cmn_linked_list_init(&list);
	ASSERT_EQ(cmn_list_is_empty(&(list.super)), true);
}

TEST(cmn_linked_list, non_empty_after_single_insertion) {
	struct cmn_linked_list list;
	cmn_linked_list_init(&list);
	cmn_list_push_back(&(list.super), (void*)1);
	ASSERT_EQ(cmn_list_is_empty(&(list.super)), false);
}

TEST(cmn_linked_list, empty_after_single_deletion_after_single_insertion) {
	struct cmn_linked_list list;
	cmn_linked_list_init(&list);
	cmn_list_push_back(&(list.super), (void*)1);
	cmn_list_pop_back(&(list.super));
	ASSERT_EQ(cmn_list_is_empty(&(list.super)), true);
}

TEST(cmn_linked_list, item_inserted_is_retrievable) {
	void* inserted = (void*)1;
	void* retrieved = NULL;
	struct cmn_linked_list list;
	cmn_linked_list_init(&list);
	cmn_list_push_back(&(list.super), inserted);
	retrieved = cmn_list_front(&(list.super));
	ASSERT_EQ(inserted, retrieved);
}
