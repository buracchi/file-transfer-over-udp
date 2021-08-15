#include <gtest/gtest.h>

extern "C" {
#include "map.h"
#include "map/linked_list_map.h"
}

TEST(cmn_linked_list_map, empty_after_initialization) {
	struct cmn_linked_list_map map;
	cmn_linked_list_map_init(&map);
	ASSERT_EQ(cmn_map_is_empty(&(map.super)), true);
}

TEST(cmn_linked_list_map, non_empty_after_single_insertion) {
	struct cmn_linked_list_map map;
	cmn_linked_list_map_init(&map);
	cmn_map_insert(&(map.super), (void*)'a', (void*)1);
	ASSERT_EQ(cmn_map_is_empty(&(map.super)), false);
}

TEST(cmn_linked_list_map, empty_after_clean_after_insertion) {
	struct cmn_linked_list_map map;
	cmn_linked_list_map_init(&map);
	cmn_map_insert(&(map.super), (void*)'a', (void*)1);
	cmn_map_clear(&(map.super));
	ASSERT_EQ(cmn_map_is_empty(&(map.super)), true);
}

TEST(cmn_linked_list_map, item_inserted_is_retrievable) {
	void* inserted = (void*)1;
	void* retrieved = NULL;
	struct cmn_linked_list_map map;
	cmn_linked_list_map_init(&map);
	cmn_map_insert(&(map.super), (void*)'a', inserted);
	cmn_map_at(&(map.super), (void*)'a', &retrieved);
	ASSERT_EQ(inserted, retrieved);
}
