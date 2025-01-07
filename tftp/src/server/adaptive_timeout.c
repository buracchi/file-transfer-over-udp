#include "adaptive_timeout.h"

#include <limits.h>
#include <math.h>

// See RFC 6298 for reference

constexpr double alpha = 1.0 / 8.0;
constexpr double beta = 1.0 / 4.0;
constexpr double k = 4.0;
constexpr double granularity = 1e-5; // 10,000 ns

static inline double timespec_to_double(struct timespec t) {
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

static inline struct timespec double_to_timespec(double d) {
    constexpr struct timespec *t [[maybe_unused]] = nullptr;
    constexpr typeof(t->tv_sec) max_sec = ((1ULL << (sizeof(t->tv_sec) * CHAR_BIT - 1)) - 1);
    constexpr double max_timeout = (double)max_sec + 1.0 - 1e-9;
    if (d > max_timeout) {
        d = max_timeout;
    }
    if (d < 1e-9) {
        d = 0.0;
    }
    struct timespec ts = {
        .tv_sec = (typeof(ts.tv_sec)) d,
        .tv_nsec = (typeof(ts.tv_nsec)) ((d - (double) ((typeof(ts.tv_sec)) d)) * 1e9),
    };
    if (ts.tv_nsec <= -1'000'000'000LL || ts.tv_nsec >= 1'000'000'000LL) {
        ts.tv_sec += ts.tv_nsec / 1'000'000'000LL;
        ts.tv_nsec %= 1'000'000'000LL;
    }
    if(ts.tv_nsec < 0) {
        --ts.tv_sec;
        ts.tv_nsec = 1'000'000'000LL + ts.tv_nsec;
    }
    return ts;
}

void adaptive_timeout_init(struct adaptive_timeout at[static 1]) {
    *at = (struct adaptive_timeout) {
        .is_first_measurement = true,
        .rto = {.tv_sec = 0, .tv_nsec = 3'906'250}, // 2^-8 s about 1000 times the average RTT of a LAN
    };
}

bool adaptive_timeout_start_timer(struct adaptive_timeout at[static 1]) {
    at->is_timer_active = true;
    return clock_gettime(CLOCK_MONOTONIC, &at->timer) == 0;
}

bool adaptive_timeout_stop_timer(struct adaptive_timeout at[static 1]) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
        return false;
    }
    at->is_timer_active = false;

    const double r = timespec_to_double(now) - timespec_to_double(at->timer);
    if (at->is_first_measurement) {
        at->is_first_measurement = false;

        at->smoothed_rtt = r;
        at->rtt_variation = r / 2.0;
    } else {
        at->rtt_variation = (1.0 - beta) * at->rtt_variation + beta * fabs(at->smoothed_rtt - r);
        at->smoothed_rtt = (1.0 - alpha) * at->smoothed_rtt + alpha * r;
    }

    const double rto = at->smoothed_rtt + fmax(granularity, k * at->rtt_variation);
    at->rto = double_to_timespec(fmin(60.0, rto));
    return true;
}

void adaptive_timeout_backoff(struct adaptive_timeout at[static 1]) {
    double rto = fmin(60.0, 2 * timespec_to_double(at->rto));
    at->rto = double_to_timespec(rto);
}
