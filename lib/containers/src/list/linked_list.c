#include "list/linked_list.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "iterator.h"
#include "types/list/linked_list.h"
#include "linked_list_element.h"
#include "linked_list_iterator.h"
#include "try.h"

static struct cmn_list_vtbl* get_list_vtbl();
static int destroy(cmn_list_t list);
static void* front(cmn_list_t list);
static void* back(cmn_list_t list);
static cmn_iterator_t begin(cmn_list_t list);
static cmn_iterator_t end(cmn_list_t list);
static bool is_empty(cmn_list_t list);
static size_t size(cmn_list_t list);
static void clear(cmn_list_t list);
static int insert(cmn_list_t list, cmn_iterator_t position, void* value, cmn_iterator_t* inserted);
static int erase(cmn_list_t list, cmn_iterator_t position, cmn_iterator_t* iterator);
static int push_front(cmn_list_t list, void* value);
static int push_back(cmn_list_t list, void* value);
static void pop_front(cmn_list_t list);
static void pop_back(cmn_list_t list);
static int resize(cmn_list_t list, size_t s, void* value);
static void swap(cmn_list_t list, cmn_list_t other);
static int merge(cmn_list_t list, cmn_list_t other, int(*comp)(void* a, void* b, bool* result));
static void splice(cmn_list_t list, cmn_list_t other, cmn_iterator_t pos, cmn_iterator_t first, cmn_iterator_t last);
static size_t remove(cmn_list_t list, void* value);
static int remove_if(cmn_list_t list, int(*p)(void* a, bool* result), size_t* removed);
static void reverse(cmn_list_t list);
static int unique(cmn_list_t list, int(*comp)(void* a, void* b, bool* result), size_t* removed);
static int sort(cmn_list_t list, int(*comp)(void* a, void* b, bool* result));

/*******************************************************************************
*                               Member functions                               *
*******************************************************************************/

extern cmn_linked_list_t cmn_linked_list_init() {
	cmn_linked_list_t list;
	try(list = malloc(sizeof *list), NULL, fail);
	try(cmn_linked_list_ctor(list), 1, fail);
	return list;
fail2:
	free(list);
fail:
	return NULL;
}

extern int cmn_linked_list_ctor(cmn_linked_list_t list) {
	memset(list, 0, sizeof *list);
	list->super.__ops_vptr = get_list_vtbl();
	list->size = 0;
	list->head = NULL;
	list->tail = NULL;
	return 0;
}

static struct cmn_list_vtbl* get_list_vtbl() {
	struct cmn_list_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__cmn_list_ops_vtbl, sizeof vtbl_zero)) {
		__cmn_list_ops_vtbl.destroy = destroy;
		__cmn_list_ops_vtbl.front = front;
		__cmn_list_ops_vtbl.back = back;
		__cmn_list_ops_vtbl.begin = begin;
		__cmn_list_ops_vtbl.end = end;
		__cmn_list_ops_vtbl.is_empty = is_empty;
		__cmn_list_ops_vtbl.size = size;
		__cmn_list_ops_vtbl.clear = clear;
		__cmn_list_ops_vtbl.insert = insert;
		__cmn_list_ops_vtbl.erase = erase;
		__cmn_list_ops_vtbl.push_front = push_front;
		__cmn_list_ops_vtbl.push_back = push_back;
		__cmn_list_ops_vtbl.pop_front = pop_front;
		__cmn_list_ops_vtbl.pop_back = pop_back;
		__cmn_list_ops_vtbl.resize = resize;
		__cmn_list_ops_vtbl.swap = swap;
		__cmn_list_ops_vtbl.merge = merge;
		__cmn_list_ops_vtbl.splice = splice;
		__cmn_list_ops_vtbl.remove = remove;
		__cmn_list_ops_vtbl.remove_if = remove_if;
		__cmn_list_ops_vtbl.reverse = reverse;
		__cmn_list_ops_vtbl.unique = unique;
		__cmn_list_ops_vtbl.sort = sort;
	}
	return &__cmn_list_ops_vtbl;
}

static int destroy(cmn_list_t list) {
	clear(list);
	return 0;
}

/*******************************************************************************
*                                Element access                                *
*******************************************************************************/

static void* front(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	return _this->head->data;
}

static void* back(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	return _this->tail->data;
}

/*******************************************************************************
*                                   Iterators                                  *
*******************************************************************************/

static cmn_iterator_t begin(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	return &(linked_list_iterator_init(_this, _this->head)->super);
}

static cmn_iterator_t end(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	return &(linked_list_iterator_init(_this, NULL)->super);
}

/*******************************************************************************
*                                   Capacity                                   *
*******************************************************************************/

static bool is_empty(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	return _this->size == 0;
}

static size_t size(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	return _this->size;
}

/*******************************************************************************
*                                   Modifiers                                  *
*******************************************************************************/

static void clear(cmn_list_t list) {
	while (!is_empty(list)) {
		pop_front(list);
	}
}

static int insert(cmn_list_t list, cmn_iterator_t position, void* value, cmn_iterator_t* inserted) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	struct linked_list_iterator* _position = (struct linked_list_iterator *)position;
	struct list_element* new_element;
	new_element = list_element_init(value, _position->element);
	if (new_element) {
		if (_position->element == _this->head) {
			if (_this->size == 0) {
				_this->tail = new_element;
			}
			_this->head = new_element;
		}
		else {
			struct list_element* prev;
			prev = get_previous_element(_this, _position->element);
			prev->next = new_element;
		}
		_this->size++;
		if (inserted) {
			*inserted = &(linked_list_iterator_init(_this, new_element)->super);
		}
	}
	return 1;
}

static int erase(cmn_list_t list, cmn_iterator_t position, cmn_iterator_t* iterator) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	struct linked_list_iterator* _position = (struct linked_list_iterator *)position;

	struct list_element* follower;
	follower = _position->element->next;

	if (_position->element == _this->head) {
		struct list_element* old_head;
		old_head = _this->head;
		_this->head = follower;
		if (_this->head == NULL) {
			_this->tail = NULL;
		}
		element_destroy(old_head);
	}
	else {
		struct list_element* prev;
		prev = get_previous_element(_this, _position->element);
		prev->next = follower;
		if (_position->element == _this->tail) {
			_this->tail = prev;
		}
		element_destroy(_position->element);
	}
	_this->size--;
	if (iterator) {
		*iterator = &(linked_list_iterator_init(_this, follower)->super);
	}
	return 0;
}

static int push_front(cmn_list_t list, void* value) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;

	struct list_element* new_element;
	new_element = list_element_init(value, _this->head);
	if (new_element) {
		_this->head = new_element;
		if (_this->size == 0) {
			_this->tail = new_element;
		}
		_this->size++;
		return 0;
	}
	return ENOMEM;
}

static int push_back(cmn_list_t list, void* value) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;

	struct list_element* new_element;
	new_element = list_element_init(value, NULL);
	if (new_element) {
		if (_this->size == 0) {
			_this->head = new_element;
		}
		else {
			((struct list_element*)_this->tail)->next = new_element;
		}
		_this->tail = new_element;
		_this->size++;
		return 0;
	}
	return ENOMEM;
}

static void pop_front(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;

	if (_this->size == 1) {
		element_destroy(_this->head);
		_this->head = NULL;
		_this->tail = NULL;
	}
	else {
		struct list_element* old_head;
		old_head = _this->head;
		_this->head = old_head->next;
		element_destroy(old_head);
	}
	_this->size--;
}

static void pop_back(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;

	if (_this->size == 1) {
		element_destroy(_this->head);
		_this->head = NULL;
		_this->tail = NULL;
	}
	else {
		struct list_element* prev;
		prev = get_previous_element(_this, _this->tail);
		prev->next = NULL;
		element_destroy(_this->tail);
		_this->tail = prev;
	}
	_this->size--;
}

static int resize(cmn_list_t list, size_t s, void* value) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;

	int ret = 0;
	while (_this->size != s) {
		if (_this->size > s) {
			pop_back(list);
		}
		else if ((ret = push_back(list, value))) {
			return ret;
		}
	}
	return 0;
}

static void swap(cmn_list_t list, cmn_list_t other) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	cmn_linked_list_t _other = (cmn_linked_list_t)other;

	struct cmn_linked_list tmp;
	tmp.size = _this->size;
	tmp.head = _this->head;
	tmp.tail = _this->tail;

	_this->head = _other->head;
	_this->size = _other->size;
	_this->tail = _other->tail;
	_other->head = tmp.head;
	_other->size = tmp.size;
	_other->tail = tmp.tail;
}

/*******************************************************************************
*                                  Operations                                  *
*******************************************************************************/

/*
* TODO: implement common_linked_list_sort()
*/
static int merge(cmn_list_t list, cmn_list_t other, int(*comp)(void* a, void* b, bool* result)) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	cmn_linked_list_t _other = (cmn_linked_list_t)other;

	((struct list_element*)_this->tail)->next = _other->head;
	_this->size += _other->size;
	_other->head = NULL;
	_other->tail = NULL;
	_other->size = 0;
	return sort(list, comp);
}

static void splice(cmn_list_t list, cmn_list_t other, cmn_iterator_t position, cmn_iterator_t first, cmn_iterator_t last) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;
	cmn_linked_list_t _other = (cmn_linked_list_t)other;
	struct linked_list_iterator* _position = (struct linked_list_iterator *)position;
	struct linked_list_iterator* _first = (struct linked_list_iterator*)first;
	struct linked_list_iterator* _last = (struct linked_list_iterator*)last;
	if (_position->element == _this->head) {
		_this->head = _first->element;
	}
	else {
		get_previous_element(_this, _position->element)->next = _first->element;
	}
	if (_first->element == _other->head) {
		_other->head = _last->element;
	}
	else {
		get_previous_element(_other, _first->element)->next = _last->element;
	}
	get_previous_element(_other, _last->element)->next = _position->element;
}

static size_t remove(cmn_list_t list, void* value) {
	struct linked_list_iterator* iterator;
	struct linked_list_iterator* tmp;
	int _removed = 0;

	iterator = (struct linked_list_iterator*)begin(list);
	while (!cmn_iterator_end(&(iterator->super))) {
		if (iterator->element->data == value) {
			tmp = iterator;
			erase(list, &(iterator->super), (cmn_iterator_t*)&iterator);
			cmn_iterator_destroy(&(tmp->super));
			_removed++;
		}
		else {
			cmn_iterator_next(&(iterator->super));
		}
	}
	cmn_iterator_destroy(&(iterator->super));
	return _removed;
}

static int remove_if(cmn_list_t list, int(*pred)(void* a, bool* result), size_t* removed) {
	struct linked_list_iterator* iterator;
	struct linked_list_iterator* tmp;
	bool result;
	size_t _removed = 0;

	iterator = (struct linked_list_iterator*)begin(list);
	while (!cmn_iterator_end(&(iterator->super))) {
		if (pred(iterator->element->data, &result)) {
			return 1;
		}
		if (result) {
			tmp = iterator;
			erase(list, &(iterator->super), (cmn_iterator_t*)&iterator);
			cmn_iterator_destroy(&(tmp->super));
			_removed++;
		}
		else {
			cmn_iterator_next(&(iterator->super));
		}
	}
	cmn_iterator_destroy(&(iterator->super));
	if (removed) {
		*removed = _removed;
	}
	return 0;
}

static void reverse(cmn_list_t list) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;

	if (_this->size == 0) {
		return;
	}

	struct list_element* walk;
	struct list_element* new_head;
	walk = _this->head;
	while (walk->next) {
		new_head = walk->next;
		walk->next = new_head->next;
		new_head->next = _this->head;
		_this->head = new_head;
	}
	_this->tail = walk;
}

static inline int default_compeq(void* a, void* b, bool* result) {
	*result = (a == b);
	return 0;
}

static int unique(cmn_list_t list, int(*comp)(void* a, void* b, bool* result), size_t* removed) {
	cmn_linked_list_t _this = (cmn_linked_list_t)list;

	struct linked_list_iterator* iterator1;
	struct linked_list_iterator* iterator2;
	struct linked_list_iterator* tmp;
	bool result;
	size_t _removed = 0;

	if (!comp) {
		comp = default_compeq;
	}
	iterator1 = (struct linked_list_iterator*)begin(list);
	iterator2 = linked_list_iterator_init(_this, NULL);
	while (!cmn_iterator_end(&(iterator1->super))) {
		iterator2->element = iterator1->element->next;
		while (!cmn_iterator_end(&(iterator2->super))) {
			int ret;
			void* e1 = cmn_iterator_data(&(iterator1->super));
			void* e2 = cmn_iterator_data(&(iterator2->super));
			ret = comp(e1, e2, &result);
			if (ret) {
				return ret;
			}
			if (result) {
				tmp = iterator2;
				erase(list, &(iterator2->super), (cmn_iterator_t*)&iterator2);
				cmn_iterator_destroy(&(tmp->super));
				_removed++;
			}
			else {
				cmn_iterator_next(&(iterator2->super));
			}
		}
		cmn_iterator_next(&(iterator1->super));
	}
	cmn_iterator_destroy(&(iterator1->super));
	cmn_iterator_destroy(&(iterator2->super));

	if (removed) {
		*removed = _removed;
	}
	return 0;
}

static inline int default_complt(void* a, void* b, bool* result) {
	*result = (a < b);
	return 0;
}

/*
* TODO: it needs an algorithm library
*/
static int sort(cmn_list_t list, int (*comp)(void* a, void* b, bool* result)) {
	return 1;
}
