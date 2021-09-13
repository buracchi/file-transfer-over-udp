#include "queue.h"
#include "queue/double_linked_list_stack_queue.h"

#include <stdlib.h>
#include <string.h>

#include "stack.h"
#include "types/queue.h"
#include "types/queue/double_linked_list_stack_queue.h"
#include "try.h"

static struct cmn_queue_vtbl *get_queue_vtbl();

static int destroy(cmn_queue_t queue);

static bool is_empty(cmn_queue_t queue);

static int enqueue(cmn_queue_t queue, void *item);

static void *dequeue(cmn_queue_t queue);

extern cmn_double_linked_list_stack_queue_t cmn_double_linked_list_stack_queue_init() {
    cmn_double_linked_list_stack_queue_t queue;
    try(queue = malloc(sizeof *queue), NULL, fail);
    try(cmn_double_linked_list_stack_queue_ctor(queue), 1, fail2);
    return queue;
fail2:
    free(queue);
fail:
    return NULL;
}

extern int cmn_double_linked_list_stack_queue_ctor(cmn_double_linked_list_stack_queue_t queue) {
    memset(queue, 0, sizeof *queue);
    queue->super.__ops_vptr = get_queue_vtbl();
    try(queue->stack_in = cmn_linked_list_stack_init(), NULL, fail);
    try(queue->stack_out = cmn_linked_list_stack_init(), NULL, fail2);
    return 0;
fail2:
    cmn_stack_destroy((cmn_stack_t) queue->stack_in);
fail:
    return 1;
}

static struct cmn_queue_vtbl *get_queue_vtbl() {
    struct cmn_queue_vtbl vtbl_zero = {0};
    if (!memcmp(&vtbl_zero, &__cmn_queue_ops_vtbl, sizeof vtbl_zero)) {
        __cmn_queue_ops_vtbl.destroy = destroy;
        __cmn_queue_ops_vtbl.is_empty = is_empty;
        __cmn_queue_ops_vtbl.enqueue = enqueue;
        __cmn_queue_ops_vtbl.dequeue = dequeue;
    }
    return &__cmn_queue_ops_vtbl;
}

static int destroy(cmn_queue_t queue) {
    struct cmn_double_linked_list_stack_queue *this = (struct cmn_double_linked_list_stack_queue *) queue;
    return cmn_stack_destroy((cmn_stack_t) this->stack_in) || cmn_stack_destroy((cmn_stack_t) this->stack_out);
}

static bool is_empty(cmn_queue_t queue) {
    struct cmn_double_linked_list_stack_queue *this = (struct cmn_double_linked_list_stack_queue *) queue;
    return cmn_stack_is_empty((cmn_stack_t) this->stack_in) && cmn_stack_is_empty((cmn_stack_t) this->stack_out);
}

static int enqueue(cmn_queue_t queue, void *item) {
    struct cmn_double_linked_list_stack_queue *this = (struct cmn_double_linked_list_stack_queue *) queue;
    return cmn_stack_push((cmn_stack_t) this->stack_in, item);
}

static void *dequeue(cmn_queue_t queue) {
    struct cmn_double_linked_list_stack_queue *this = (struct cmn_double_linked_list_stack_queue *) queue;
    if (cmn_stack_is_empty((cmn_stack_t) this->stack_out)) {
        while (!cmn_stack_is_empty((cmn_stack_t) this->stack_in)) {
            cmn_stack_push((cmn_stack_t) this->stack_out, cmn_stack_pop((cmn_stack_t) this->stack_in));
        }
    }
    return cmn_stack_pop((cmn_stack_t) this->stack_out);
}
