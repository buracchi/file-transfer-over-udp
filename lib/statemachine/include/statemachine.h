#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef struct cmn_statemachine *cmn_statemachine_t;

typedef struct cmn_statemachine_context *cmn_statemachine_context_t;

typedef int cmn_statemachine_event_t;

struct cmn_statemachine_state {
    void (*entry)(cmn_statemachine_context_t context);

    void (*exit)(cmn_statemachine_context_t context);
};

struct cmn_statemachine_transition {
    struct cmn_statemachine_state *source;
    cmn_statemachine_event_t trigger;

    bool (*guard)(cmn_statemachine_context_t context);

    void (*effect)(cmn_statemachine_context_t context);

    struct cmn_statemachine_state *target;
};

struct cmn_statemachine {
    struct cmn_statemachine_state *state;
    struct cmn_statemachine_transition *transitions;
    size_t transitions_number;
};

struct cmn_statemachine_context {
    cmn_statemachine_t statemachine;
    struct cmn_statemachine_transition *transition;
};

extern cmn_statemachine_t cmn_statemachine_init(struct cmn_statemachine_state *initial_state,
                                                struct cmn_statemachine_transition *transitions,
                                                size_t transitions_number);

extern void cmn_statemachine_destroy(cmn_statemachine_t statemachine);

extern void cmn_statemachine_send_event(cmn_statemachine_t statemachine, cmn_statemachine_event_t event);
