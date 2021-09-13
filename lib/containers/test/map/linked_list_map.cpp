#include <gtest/gtest.h>

extern "C" {
#include "map/linked_list_map.h"
}

TEST(cmn_linked_list_map, empty_after_initialization) {
    bool is_empty;
    auto map = (cmn_map_t) cmn_linked_list_map_init();
    is_empty = cmn_map_is_empty(map);
    cmn_map_destroy(map);
    ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list_map, non_empty_after_single_insertion) {
    bool is_empty;
    auto map = (cmn_map_t) cmn_linked_list_map_init();
    cmn_map_insert(map, (void *) 'a', (void *) 1, nullptr);
    is_empty = cmn_map_is_empty(map);
    cmn_map_destroy(map);
    ASSERT_EQ(is_empty, false);
}

TEST(cmn_linked_list_map, empty_after_clean_after_insertion) {
    bool is_empty;
    auto map = (cmn_map_t) cmn_linked_list_map_init();
    cmn_map_insert(map, (void *) 'a', (void *) 1, nullptr);
    cmn_map_clear(map);
    is_empty = cmn_map_is_empty(map);
    cmn_map_destroy(map);
    ASSERT_EQ(is_empty, true);
}

TEST(cmn_linked_list_map, item_inserted_is_retrievable) {
    void *inserted = (void *) 1;
    void *retrieved = nullptr;
    auto map = (cmn_map_t) cmn_linked_list_map_init();
    cmn_map_insert(map, (void *) 'a', inserted, nullptr);
    cmn_map_at(map, (void *) 'a', &retrieved);
    cmn_map_destroy(map);
    ASSERT_EQ(inserted, retrieved);
}
