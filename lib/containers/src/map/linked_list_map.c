#include "map.h"
#include "map/linked_list_map.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "utilities.h"
#include "try.h"

#define KEY first
#define VALUE second

static struct cmn_map_vtbl* get_map_vtbl();
static int destroy(struct cmn_map* map);
static int at(struct cmn_map* map, void* key, void** value);
static struct cmn_iterator* begin(struct cmn_map* map);
static struct cmn_iterator* end(struct cmn_map* map);
static bool is_empty(struct cmn_map* map);
static size_t size(struct cmn_map* map);
static void clear(struct cmn_map* map);
static struct cmn_iterator* insert(struct cmn_map* map, void* key, void* value);
static struct cmn_iterator* insert_or_assign(struct cmn_map* map, void* key, void* value);
static struct cmn_iterator* erase(struct cmn_map* map, struct cmn_iterator* position);
static void swap(struct cmn_map* map, struct cmn_map* other);
static int count(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), size_t* count);
static int find(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), struct cmn_iterator** iterator);

extern int cmn_linked_list_map_init(struct cmn_linked_list_map* map) {
	memset(map, 0, sizeof *map);
	map->super.__ops_vptr = get_map_vtbl();
	return cmn_linked_list_init(&(map->list));
}

static struct cmn_map_vtbl* get_map_vtbl() {
	struct cmn_map_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__cmn_map_ops_vtbl, sizeof vtbl_zero)) {
		__cmn_map_ops_vtbl.destroy = destroy;
		__cmn_map_ops_vtbl.at = at;
		__cmn_map_ops_vtbl.begin = begin;
		__cmn_map_ops_vtbl.end = end;
		__cmn_map_ops_vtbl.is_empty = is_empty;
		__cmn_map_ops_vtbl.size = size;
		__cmn_map_ops_vtbl.clear = clear;
		__cmn_map_ops_vtbl.insert = insert;
		__cmn_map_ops_vtbl.insert_or_assign = insert_or_assign;
		__cmn_map_ops_vtbl.erase = erase;
		__cmn_map_ops_vtbl.swap = swap;
		__cmn_map_ops_vtbl.count = count;
		__cmn_map_ops_vtbl.find = find;
	}
	return &__cmn_map_ops_vtbl;
}

static int destroy(struct cmn_map* map) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	clear(map);
	return cmn_list_destroy(&(_this->list.super));
}

static int at(struct cmn_map* map, void* key, void** value) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	struct cmn_iterator* iterator;
	iterator = cmn_list_begin(&(_this->list.super));
	while (cmn_iterator_end(iterator) == false) {
		struct cmn_pair* kv_pair = (struct cmn_pair*)cmn_iterator_data(iterator);
		if (kv_pair->KEY == key) {
			*value = kv_pair->VALUE;
			cmn_iterator_destroy(iterator);
			return 0;
		}
		iterator = cmn_iterator_next(iterator);
	}
	cmn_iterator_destroy(iterator);
	errno = ERANGE;
	return 1;
}

static struct cmn_iterator* begin(struct cmn_map* map) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	return cmn_list_begin(&(_this->list.super));
}

static struct cmn_iterator* end(struct cmn_map* map) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	return cmn_list_end(&(_this->list.super));
}

static bool is_empty(struct cmn_map* map) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	return cmn_list_is_empty(&(_this->list.super));
}

static size_t size(struct cmn_map* map) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	return cmn_list_size(&(_this->list.super));
}

static void clear(struct cmn_map* map) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	struct cmn_iterator* iterator;
	iterator = cmn_list_begin(&(_this->list.super));
	while (cmn_iterator_end(iterator) == false) {
		struct cmn_pair* kv_pair = (struct cmn_pair*)cmn_iterator_data(iterator);
		free(kv_pair);
		iterator = cmn_iterator_next(iterator);
	}
	cmn_iterator_destroy(iterator);
	cmn_list_clear(&(_this->list.super));
}

static struct cmn_iterator* insert(struct cmn_map* map, void* key, void* value) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	struct cmn_iterator* result = NULL;
	struct cmn_pair* kv_pair = malloc(sizeof * kv_pair);
	if (kv_pair) {
		kv_pair->KEY = key;
		kv_pair->VALUE = value;
		struct cmn_iterator* iterator = cmn_list_begin(&(_this->list.super));
		result = cmn_list_insert(&(_this->list.super), iterator, kv_pair);
		cmn_iterator_destroy(iterator);
	}
	return result;
}

static struct cmn_iterator* insert_or_assign(struct cmn_map* map, void* key, void* value) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	struct cmn_pair* kv_pair;
	if (at(map, key, (void**)&kv_pair)) {
		return insert(map, key, value);
	}
	kv_pair->VALUE = value;
	struct cmn_iterator* iterator = cmn_list_begin(&(_this->list.super));
	while (((struct cmn_pair*)cmn_iterator_data(iterator))->KEY != key) {
		iterator = cmn_iterator_next(iterator);
	}
	return iterator;
}

static struct cmn_iterator* erase(struct cmn_map* map, struct cmn_iterator* position) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	struct cmn_pair* kv_pair = (struct cmn_pair*)cmn_iterator_data(position);
	free(kv_pair);
	return cmn_list_erase(&(_this->list.super), position);
}

static void swap(struct cmn_map* map, struct cmn_map* other) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	struct cmn_linked_list_map* _other = (struct cmn_linked_list_map*) other;
	cmn_list_swap(&(_this->list.super), &(_other->list.super));
}

static int count(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), size_t* count) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	struct cmn_iterator* iterator;
	*count = 0;
	iterator = cmn_list_begin(&(_this->list.super));
	while (cmn_iterator_end(iterator) == false) {
		struct cmn_pair* kv_pair = (struct cmn_pair*)cmn_iterator_data(iterator);
		bool found;
		try(comp(kv_pair->KEY, key, &found), !0, fail);
		if (found) {
			*count = 1;
			break;
		}
		iterator = cmn_iterator_next(iterator);
	}
	cmn_iterator_destroy(iterator);
	return 0;
fail:
	cmn_iterator_destroy(iterator);
	return 1;
}

static int find(struct cmn_map* map, void* key, int (*comp)(void* a, void* b, bool* result), struct cmn_iterator** iterator) {
	struct cmn_linked_list_map* _this = (struct cmn_linked_list_map*) map;
	struct cmn_iterator* i;
	i = cmn_list_begin(&(_this->list.super));
	while (cmn_iterator_end(i) == false) {
		struct cmn_pair* kv_pair = (struct cmn_pair*)cmn_iterator_data(i);
		bool found;
		try(comp(kv_pair->KEY, key, &found), !0, fail);
		if (found) {
			*iterator = i;
			return 0;
		}
		i = cmn_iterator_next(i);
	}
fail:
	cmn_iterator_destroy(i);
	return 1;
}
