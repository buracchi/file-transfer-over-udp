#include "stack.h"
#include "stack/linked_list_stack.h"

#include <stdlib.h>
#include <string.h>

#include "list.h"

static struct cmn_stack_vtbl* get_stack_vtbl();
static int destroy(struct cmn_stack* stack);
static void* peek(struct cmn_stack* stack);
static bool is_empty(struct cmn_stack* stack);
static size_t get_size(struct cmn_stack* stack);
static int push(struct cmn_stack* stack, void* item);
static void* pop(struct cmn_stack* stack);

extern int cmn_linked_list_stack_init(struct cmn_linked_list_stack* stack) {
	memset(stack, 0, sizeof *stack);
	stack->super.__ops_vptr = get_stack_vtbl();
	return cmn_linked_list_init(&(stack->list));
}

static struct cmn_stack_vtbl* get_stack_vtbl() {
	struct cmn_stack_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__cmn_stack_ops_vtbl, sizeof vtbl_zero)) {
		__cmn_stack_ops_vtbl.destroy = destroy;
		__cmn_stack_ops_vtbl.peek = peek;
		__cmn_stack_ops_vtbl.is_empty = is_empty;
		__cmn_stack_ops_vtbl.get_size = get_size;
		__cmn_stack_ops_vtbl.push = push;
		__cmn_stack_ops_vtbl.pop = pop;
	}
	return &__cmn_stack_ops_vtbl;
}

static int destroy(struct cmn_stack* stack) {
	struct cmn_linked_list_stack* this = (struct cmn_linked_list_stack*) stack;
	return cmn_list_destroy(&(this->list.super));
}

static void* peek(struct cmn_stack* stack) {
	struct cmn_linked_list_stack* this = (struct cmn_linked_list_stack*) stack;
	return cmn_list_front(&(this->list.super));
}

static bool is_empty(struct cmn_stack* stack) {
	struct cmn_linked_list_stack* this = (struct cmn_linked_list_stack*) stack;
	return cmn_list_is_empty(&(this->list.super));
}

static size_t get_size(struct cmn_stack* stack) {
	struct cmn_linked_list_stack* this = (struct cmn_linked_list_stack*) stack;
	return cmn_list_size(&(this->list.super));
}

static int push(struct cmn_stack* stack, void* item) {
	struct cmn_linked_list_stack* this = (struct cmn_linked_list_stack*) stack;
	return cmn_list_push_front(&(this->list.super), item);
}

static void* pop(struct cmn_stack* stack) {
	struct cmn_linked_list_stack* this = (struct cmn_linked_list_stack*) stack;
	void* popped;
	popped = cmn_list_front(&(this->list.super));
	cmn_list_pop_front(&(this->list.super));
	return popped;
}
