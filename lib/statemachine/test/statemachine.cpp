#include <gtest/gtest.h>

extern "C" {
#include "statemachine.h"
}

//          +----------+     EV1/x++    +----------+
//          |  stateA  |--------------->|  stateB  |<--+
//  - - - ->|----------| EV1 [x even]/  |----------|   | EV2/x++
//          |entry/x++ |<---------------|exit/x++  |---+
//          +----------+                +----------+

enum event {
    EV1,
    EV2
};

static bool is_x_even(cmn_statemachine_context_t context);

static void increment_x(cmn_statemachine_context_t context);

static struct cmn_statemachine_state stateA = {&increment_x, nullptr};
static struct cmn_statemachine_state stateB = {nullptr, &increment_x};
static struct cmn_statemachine_transition transitions[] = {
        {&stateA, EV1, nullptr,    &increment_x, &stateB},
        {&stateB, EV1, &is_x_even, nullptr,      &stateA},
        {&stateB, EV2, nullptr,    &increment_x, &stateB}
};
static const size_t transitions_number = sizeof transitions / sizeof *transitions;

static int x = 0;

TEST(cmn_statemachine, test) {
    cmn_statemachine_t statemachine = cmn_statemachine_init(&stateA, transitions, transitions_number);
    cmn_statemachine_send_event(statemachine, EV2);
    cmn_statemachine_send_event(statemachine, EV1);
    cmn_statemachine_send_event(statemachine, EV2);
    cmn_statemachine_send_event(statemachine, EV1);
    cmn_statemachine_destroy(statemachine);
    ASSERT_EQ(x, 6);
}

static bool is_x_even(cmn_statemachine_context_t context) {
    return x % 2 ? false : true;
}

static void increment_x(cmn_statemachine_context_t context) {
    x++;
}
