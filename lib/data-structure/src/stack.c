#include <stack.h>
#include <stdlib.h>

struct stack_node {
	void* data;
	struct stack_node* next;
};

extern _stack_t stack_init() {
	struct stack_node* stack;
	stack = malloc(sizeof * stack);
	if (stack) {
		stack->data = NULL;
		stack->next = NULL;
	}
	return stack;
}

extern void stack_destroy(const _stack_t handle) {
	struct stack_node* _this = handle;
	while (!stack_is_empty(_this)) {
		stack_pop(_this);
	}
	free(_this);
}

extern void* stack_peek(const _stack_t handle) {
	struct stack_node* _this = handle;
	return _this->data;
}

extern bool stack_is_empty(const _stack_t handle) {
	struct stack_node* _this = handle;
	return !_this->next;
}

extern size_t stack_get_size(const _stack_t handle) {
	struct stack_node* _this = handle;
	return 0;
}

extern int stack_push(const _stack_t handle, void* item) {
	struct stack_node* _this = handle;
	struct stack_node* node;
	node = malloc(sizeof * node);
	if (node) {
		node->data = _this->data;
		node->next = _this->next;
		_this->data = item;
		_this->next = node;
		return 0;
	}
	return 1;
}

extern void* stack_pop(const _stack_t handle) {
	struct stack_node* _this = handle;
	struct stack_node* tmp = _this->next;
	void* popped = _this->data;
	if (!stack_is_empty(_this)) {
		_this->data = _this->next->data;
		_this->next = _this->next->next;
		free(tmp);
	}
	return popped;
}
