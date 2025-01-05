#ifndef ADAPTIVE_TIMEOUT_H
#define ADAPTIVE_TIMEOUT_H

#include <stdint.h>
#include <time.h>

struct adaptive_timeout {
    bool is_timer_active;
    struct timespec rto;
    uint16_t starting_block_number;
    uint16_t packets_sent;
    /* private members */
    bool is_first_measurement;
    struct timespec timer;
    double smoothed_rtt;
    double rtt_variation;
};

void adaptive_timeout_init(struct adaptive_timeout at[static 1]);

bool adaptive_timeout_start_timer(struct adaptive_timeout at[static 1]);

static inline void adaptive_timeout_cancel_timer(struct adaptive_timeout at[static 1]) {
    at->is_timer_active = false;
}

bool adaptive_timeout_stop_timer(struct adaptive_timeout at[static 1]);

void adaptive_timeout_backoff(struct adaptive_timeout at[static 1]);

#endif // ADAPTIVE_TIMEOUT_H
