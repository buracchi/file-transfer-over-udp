#include "statemachine.h"

#include <stddef.h>
#include <stdlib.h>

#include <buracchi/common/utilities/try.h>

extern cmn_statemachine_t cmn_statemachine_init(struct cmn_statemachine_state *initial_state,
                                                struct cmn_statemachine_transition *transitions, size_t transitions_number) {
    cmn_statemachine_t statemachine;
    try(statemachine = malloc(sizeof *statemachine), NULL, FAIL);
    statemachine->state = initial_state;
    statemachine->transitions = transitions;
    statemachine->transitions_number = transitions_number;
    if (statemachine->state->entry) {
        statemachine->state->entry(&(struct cmn_statemachine_context) {statemachine, NULL});
    }
    return statemachine;
FAIL:
    return NULL;
}

extern void cmn_statemachine_destroy(cmn_statemachine_t statemachine) {
    free(statemachine);
}

extern void cmn_statemachine_send_event(cmn_statemachine_t statemachine, cmn_statemachine_event_t event) {
    for (size_t i = 0; i < statemachine->transitions_number; i++) {
        if (statemachine->state != statemachine->transitions[i].source || event != statemachine->transitions[i].trigger) {
            continue;
        }
        struct cmn_statemachine_context context = {statemachine, &(statemachine->transitions[i])};
        if (!statemachine->transitions[i].guard || statemachine->transitions[i].guard(&context)) {
            if (statemachine->transitions[i].effect) {
                statemachine->transitions[i].effect(&context);
            }
            if (statemachine->state->exit) {
                statemachine->state->exit(&context);
            }
            statemachine->state = statemachine->transitions[i].target;
            if (statemachine->state->entry) {
                statemachine->state->entry(&context);
            }
        }
        break;
    }
}
