#include "../include/statemachine.h"

#include <stddef.h>
#include <stdlib.h>

#include "try.h"

struct cmn_statemachine {
    cmn_statemachine_state_t state;
    cmn_statemachine_transition_t *transitions;
    size_t transitions_number;
};

extern cmn_statemachine_t cmn_statemachine_init(cmn_statemachine_state_t initial_state,
                                                cmn_statemachine_transition_t *transitions, size_t transitions_number) {
    cmn_statemachine_t statemachine;
    try(statemachine = malloc(sizeof *statemachine), NULL, fail);
    statemachine->state = initial_state;
    statemachine->transitions = transitions;
    statemachine->transitions_number = transitions_number;
    if (statemachine->state->entry) {
        statemachine->state->entry(&(struct cmn_statemachine_context) {statemachine, NULL});
    }
    return statemachine;
fail:
    return NULL;
}

extern void cmn_statemachine_destroy(cmn_statemachine_t statemachine) {
    free(statemachine);
}

extern void cmn_statemachine_send_event(cmn_statemachine_t statemachine, cmn_statemachine_event_t event) {
    for (size_t i = 0; i < statemachine->transitions_number; i++) {
        struct cmn_statemachine_transition *transitions = *statemachine->transitions;
        if (statemachine->state == transitions[i].source && event == transitions[i].trigger) {
            struct cmn_statemachine_context context = {statemachine, &(transitions[i])};
            if (!transitions[i].guard || transitions[i].guard(&context)) {
                if (transitions[i].effect) {
                    transitions[i].effect(&context);
                }
                if (statemachine->state->exit) {
                    statemachine->state->exit(&context);
                }
                statemachine->state = transitions[i].target;
                if (statemachine->state->entry) {
                    statemachine->state->entry(&context);
                }
            }
            break;
        }
    }
}
