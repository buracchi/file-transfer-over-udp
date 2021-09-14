#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct cmn_statemachine *cmn_statemachine_t;

typedef struct cmn_statemachine_context *cmn_statemachine_context_t;

typedef int cmn_statemachine_event_t;

typedef struct cmn_statemachine_state {
    void (*entry)(cmn_statemachine_context_t context);

    void (*exit)(cmn_statemachine_context_t context);
} *cmn_statemachine_state_t;

typedef struct cmn_statemachine_transition {
    cmn_statemachine_state_t source;
    cmn_statemachine_event_t trigger;

    bool (*guard)(cmn_statemachine_context_t context);

    void (*effect)(cmn_statemachine_context_t context);

    cmn_statemachine_state_t target;
} *cmn_statemachine_transition_t;

struct cmn_statemachine_context {
    cmn_statemachine_t statemachine;
    cmn_statemachine_transition_t transition;
};

extern cmn_statemachine_t cmn_statemachine_init(cmn_statemachine_state_t initial_state,
                                                cmn_statemachine_transition_t *transitions, size_t transitions_number);

extern void cmn_statemachine_destroy(cmn_statemachine_t statemachine);

extern void cmn_statemachine_send_event(cmn_statemachine_t statemachine, cmn_statemachine_event_t event);
