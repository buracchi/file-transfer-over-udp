#include "queue/double_stack_queue.h"

#include <stdlib.h>
#include <string.h>

#include "try.h"

static struct cmn_queue_vtbl* get_queue_vtbl();
static int destroy(struct cmn_queue* queue);
static bool is_empty(struct cmn_queue* queue);
static int enqueue(struct cmn_queue* queue, void* item);
static void* dequeue(struct cmn_queue* queue);

extern int cmn_double_stack_queue_init(struct cmn_double_stack_queue* queue) {
	memset(queue, 0, sizeof *queue);
	queue->super.__ops_vptr = get_queue_vtbl();
    try(cmn_linked_list_stack_init(&(queue->stack_in)), !0, fail);
	try(cmn_linked_list_stack_init(&(queue->stack_out)), !0, fail2);
	return 0;
fail2:
	cmn_stack_destroy(&(queue->stack_in.super));
fail:
    return 1;
}

static struct cmn_queue_vtbl* get_queue_vtbl() {
	struct cmn_queue_vtbl vtbl_zero = { 0 };
	if (!memcmp(&vtbl_zero, &__cmn_queue_ops_vtbl, sizeof vtbl_zero)) {
		__cmn_queue_ops_vtbl.destroy = destroy;
		__cmn_queue_ops_vtbl.is_empty = is_empty;
		__cmn_queue_ops_vtbl.enqueue = enqueue;
		__cmn_queue_ops_vtbl.dequeue = dequeue;
	}
	return &__cmn_queue_ops_vtbl;
}

static int destroy(struct cmn_queue* queue) {
	struct cmn_double_stack_queue* this = (struct cmn_double_stack_queue*) queue;
	return cmn_stack_destroy(&(this->stack_in.super)) || cmn_stack_destroy(&(this->stack_out.super));
}

static bool is_empty(struct cmn_queue* queue) {
	struct cmn_double_stack_queue* this = (struct cmn_double_stack_queue*) queue;
	return cmn_stack_is_empty(&(this->stack_in.super)) && cmn_stack_is_empty(&(this->stack_out.super));
}

static int enqueue(struct cmn_queue* queue, void* item) {
	struct cmn_double_stack_queue* this = (struct cmn_double_stack_queue*) queue;
	return cmn_stack_push(&(this->stack_in.super), item);
}

static void* dequeue(struct cmn_queue* queue) {
	struct cmn_double_stack_queue* this = (struct cmn_double_stack_queue*) queue;
	if (cmn_stack_is_empty(&(this->stack_out.super))) {
		while (!cmn_stack_is_empty(&(this->stack_in.super))) {
			cmn_stack_push(&(this->stack_out.super), cmn_stack_pop(&(this->stack_in.super)));
		}
	}
	return cmn_stack_pop(&(this->stack_out.super));
}
