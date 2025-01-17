#ifndef TIME_H
#define TIME_H

#include <limits.h>
#include <linux/time_types.h>
#include <sys/time.h>
#include <time.h>


static inline double timespec_to_double(struct timespec t) {
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

static inline struct __kernel_timespec timespec_to_kernel_timespec(struct timespec ts) {
    return (struct __kernel_timespec) {.tv_sec = ts.tv_sec, .tv_nsec = ts.tv_nsec};
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

static inline struct timeval double_to_timeval(double d) {
    constexpr struct timeval *t [[maybe_unused]] = nullptr;
    constexpr typeof(t->tv_sec) max_sec = ((1ULL << (sizeof(t->tv_sec) * CHAR_BIT - 1)) - 1);
    constexpr double max_timeout = (double)max_sec + 1.0 - 1e-6;
    if (d > max_timeout) {
        d = max_timeout;
    }
    if (d < 1e-6) {
        d = 0.0;
    }
    struct timeval tv = {
        .tv_sec = (typeof(tv.tv_sec)) d,
        .tv_usec = (typeof(tv.tv_usec)) ((d - (double) ((typeof(tv.tv_sec)) d)) * 1e6),
    };
    if (tv.tv_usec <= -1'000'000LL || tv.tv_usec >= 1'000'000LL) {
        tv.tv_sec += tv.tv_usec / 1'000'000LL;
        tv.tv_usec %= 1'000'000LL;
    }
    if(tv.tv_usec < 0) {
        --tv.tv_sec;
        tv.tv_usec = 1'000'000LL + tv.tv_usec;
    }
    return tv;
}

#endif // TIME_H
