#include "queue.h"
#include "types/queue.h"

#include <stdbool.h>
#include <stdlib.h>

extern inline int cmn_queue_destroy(cmn_queue_t queue) {
    int ret = queue->__ops_vptr->destroy(queue);
    if (!ret) {
        free(queue);
    }
    return ret;
}

extern inline bool cmn_queue_is_empty(cmn_queue_t queue) {
    return queue->__ops_vptr->is_empty(queue);
}

extern inline int cmn_queue_enqueue(cmn_queue_t queue, void *item) {
    return queue->__ops_vptr->enqueue(queue, item);
}

extern inline void *cmn_queue_dequeue(cmn_queue_t queue) {
    return queue->__ops_vptr->dequeue(queue);
}
