#pragma once

#include <stdbool.h>

typedef void* queue_t;

queue_t queue_init();

void queue_destroy(const queue_t handle);

bool queue_is_empty(const queue_t handle);

int queue_enqueue(const queue_t handle, void* item);

void* queue_dequeue(const queue_t handle);
