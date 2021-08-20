#include "map.h"
#include "types/map.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#include "iterator.h"

extern inline int cmn_map_destroy(cmn_map_t map) {
    int ret = map->__ops_vptr->destroy(map);
	if (!ret) {
		free(map);
	}
	return ret;
}

extern inline int cmn_map_at(cmn_map_t map, void* key, void** value) {
	return map->__ops_vptr->at(map, key, value);
}

extern inline int cmn_map_at2(cmn_map_t map, void* key, int (*comp)(void* a, void* b, bool* result), void** value) {
	return map->__ops_vptr->at2(map, key, comp, value);
}

extern inline cmn_iterator_t cmn_map_begin(cmn_map_t map) {
	return map->__ops_vptr->begin(map);
}

extern inline cmn_iterator_t cmn_map_end(cmn_map_t map) {
	return map->__ops_vptr->end(map);
}

extern inline bool cmn_map_is_empty(cmn_map_t map) {
	return map->__ops_vptr->is_empty(map);
}

extern inline size_t cmn_map_size(cmn_map_t map) {
	return map->__ops_vptr->size(map);
}

extern inline void cmn_map_clear(cmn_map_t map) {
	map->__ops_vptr->clear(map);
}

extern inline int cmn_map_insert(cmn_map_t map, void* key, void* value, cmn_iterator_t* iterator) {
	return map->__ops_vptr->insert(map, key, value, iterator);
}

extern inline int cmn_map_insert_or_assign(cmn_map_t map, void* key, void* value, cmn_iterator_t* iterator) {
	return map->__ops_vptr->insert_or_assign(map, key, value, iterator);
}

extern inline int cmn_map_erase(cmn_map_t map, cmn_iterator_t position, cmn_iterator_t* iterator) {
	return map->__ops_vptr->erase(map, position, iterator);
}

extern inline void cmn_map_swap(cmn_map_t map, cmn_map_t other) {
	map->__ops_vptr->swap(map, other);
}

extern inline int cmn_map_count(cmn_map_t map, void* key, int (*comp)(void* a, void* b, bool* result), size_t* count) {
	return map->__ops_vptr->count(map, key, comp, count);
}

extern inline int cmn_map_find(cmn_map_t map, void* key, int (*comp)(void* a, void* b, bool* result), cmn_iterator_t* iterator) {
	return map->__ops_vptr->find(map, key, comp, iterator);
}
