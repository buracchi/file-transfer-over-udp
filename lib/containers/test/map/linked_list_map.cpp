#include <gtest/gtest.h>

extern "C" {
#include "map.h"
#include "map/linked_list_map.h"
}

TEST(cmn_linked_list_map, empty_after_initialization) {
	bool is_empty;
	struct cmn_linked_list_map map;
	cmn_linked_list_map_init(&map);
	is_empty = cmn_map_is_empty(&(map.super));
	cmn_map_destroy(&(map.super));
	ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list_map, non_empty_after_single_insertion) {
	bool is_empty;
	struct cmn_linked_list_map map;
	static cmn_iterator* iterator;
	cmn_linked_list_map_init(&map);
	iterator = cmn_map_insert(&(map.super), (void*)'a', (void*)1);
	cmn_iterator_destroy(iterator);
	is_empty = cmn_map_is_empty(&(map.super));
	cmn_map_destroy(&(map.super));
	ASSERT_EQ(is_empty, false);
}

TEST(cmn_linked_list_map, empty_after_clean_after_insertion) {
	bool is_empty;
	struct cmn_linked_list_map map;
	static cmn_iterator* iterator;
	cmn_linked_list_map_init(&map);
	iterator = cmn_map_insert(&(map.super), (void*)'a', (void*)1);
	cmn_iterator_destroy(iterator);
	cmn_map_clear(&(map.super));
	is_empty = cmn_map_is_empty(&(map.super));
	cmn_map_destroy(&(map.super));
	ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list_map, item_inserted_is_retrievable) {
	void* inserted = (void*)1;
	void* retrieved = NULL;
	struct cmn_linked_list_map map;
	static cmn_iterator* iterator;
	cmn_linked_list_map_init(&map);
	iterator = cmn_map_insert(&(map.super), (void*)'a', inserted);
	cmn_iterator_destroy(iterator);
	cmn_map_at(&(map.super), (void*)'a', &retrieved);
	cmn_map_destroy(&(map.super));
	ASSERT_EQ(inserted, retrieved);
}
