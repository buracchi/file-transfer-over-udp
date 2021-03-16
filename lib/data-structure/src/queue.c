#include <queue.h>
#include <stdlib.h>

#include "stack.h"
#include "try.h"

struct queue {
	_stack_t stack_in;
	_stack_t stack_out;
};

queue_t queue_init() {
	struct queue* queue;
	queue = malloc(sizeof *queue);
	if (queue) {
		try(queue->stack_in = stack_init(), NULL, fail);
		try(queue->stack_out = stack_init(), NULL, fail);
	}
	return queue;
fail:
	free(queue);
	return NULL;
}

void queue_destroy(const queue_t handle) {
	struct queue* _this = handle;
	stack_destroy(_this->stack_in);
	stack_destroy(_this->stack_out);
	free(_this);
}

bool queue_is_empty(const queue_t handle) {
	struct queue* _this = handle;
	return stack_is_empty(_this->stack_in) && stack_is_empty(_this->stack_out);
}

int queue_enqueue(const queue_t handle, void* item) {
	struct queue* _this = handle;
	return stack_push(_this->stack_in, item);
}

void* queue_dequeue(const queue_t handle) {
	struct queue* _this = handle;
	if (stack_is_empty(_this->stack_out)) {
		while (!stack_is_empty(_this->stack_in)) {
			stack_push(_this->stack_out, stack_pop(_this->stack_in));
		}
	}
	return stack_pop(_this->stack_out);
}
