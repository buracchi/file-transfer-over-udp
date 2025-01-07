#include <buracchi/cutest/cutest.h>

#include <time.h>

#include "dispatcher.h"
#include "mock_logger.h"

static inline double timespec_to_double(struct timespec t) {
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

TEST(dispatcher, register_and_dispatch_single_event) {
    struct dispatcher dispatcher;
    struct dispatcher_event event;
    ASSERT_TRUE(dispatcher_init(&dispatcher, 32, &(struct logger) {}));
    ASSERT_EQ(dispatcher.pending_requests, 0);
    ASSERT_TRUE(dispatcher_submit(&dispatcher, &event));
    ASSERT_EQ(1, dispatcher.pending_requests);
    ASSERT_TRUE(dispatcher_destroy(&dispatcher));
}

TEST(dispatcher, timeout) {
    constexpr uint64_t timeout_ns = 1'000'000;
    struct dispatcher dispatcher;
    struct dispatcher_event *event;
    struct dispatcher_event_timeout timeout_event = {.timeout = {.tv_nsec = timeout_ns}};
    ASSERT_TRUE(dispatcher_init(&dispatcher, 32, &(struct logger) {}));
    struct timespec start;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    dispatcher_submit_timeout(&dispatcher, &timeout_event);
    dispatcher_wait_event(&dispatcher, &event);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = timespec_to_double(end) - timespec_to_double(start);
    ASSERT_TRUE(elapsed >= timeout_ns / 1e9);
    ASSERT_TRUE(dispatcher_destroy(&dispatcher));
}

TEST(dispatcher, update_timeout) {
    constexpr uint64_t timeout_ns = 1'000'000;
    constexpr uint64_t updated_timeout_ns = 100'000'000;
    struct dispatcher dispatcher;
    struct dispatcher_event *event = nullptr;
    struct dispatcher_event update_event;
    struct dispatcher_event_timeout timeout_event = {.timeout = {.tv_nsec = timeout_ns}};
    ASSERT_TRUE(dispatcher_init(&dispatcher, 32, &(struct logger) {}));
    struct timespec start;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    dispatcher_submit_timeout(&dispatcher, &timeout_event);
    dispatcher_submit_timeout_update(&dispatcher,
                                     &update_event,
                                     &timeout_event,
                                     &(struct __kernel_timespec) {.tv_nsec = updated_timeout_ns});
    while (event != (struct dispatcher_event *) &timeout_event) {
        dispatcher_wait_event(&dispatcher, &event);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = timespec_to_double(end) - timespec_to_double(start);
    ASSERT_TRUE(elapsed >= updated_timeout_ns / 1e9);
    ASSERT_TRUE(dispatcher_destroy(&dispatcher));
}

TEST(dispatcher, cancel_timeout) {
    constexpr uint64_t timeout_ns = 100'000'000;
    struct dispatcher dispatcher;
    struct dispatcher_event *event = nullptr;
    struct dispatcher_event cancel_event;
    struct dispatcher_event_timeout timeout_event = {.timeout = {.tv_nsec = timeout_ns}};
    ASSERT_TRUE(dispatcher_init(&dispatcher, 32, &(struct logger) {}));
    struct timespec start;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    dispatcher_submit_timeout(&dispatcher, &timeout_event);
    dispatcher_submit_timeout_cancel(&dispatcher, &cancel_event, &timeout_event);
    while (event != (struct dispatcher_event *) &timeout_event) {
        dispatcher_wait_event(&dispatcher, &event);
    }
    ASSERT_FALSE(event->is_success);
    ASSERT_EQ(event->error_number, ECANCELED);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = timespec_to_double(end) - timespec_to_double(start);
    ASSERT_TRUE(elapsed < timeout_ns / 1e9);
    ASSERT_TRUE(dispatcher_destroy(&dispatcher));
}

TEST(dispatcher, reset_timeout) {
    constexpr uint64_t timeout_ns = 100'000'000;
    struct dispatcher dispatcher;
    struct dispatcher_event *event = nullptr;
    struct dispatcher_event cancel_event;
    struct dispatcher_event_timeout timeout_event = {.timeout = {.tv_nsec = timeout_ns}};
    ASSERT_TRUE(dispatcher_init(&dispatcher, 32, &(struct logger) {}));
    struct timespec start;
    struct timespec end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    dispatcher_submit_timeout(&dispatcher, &timeout_event);
    dispatcher_submit_timeout_cancel(&dispatcher, &cancel_event, &timeout_event);
    dispatcher_submit_timeout(&dispatcher, &timeout_event);
    while (event != (struct dispatcher_event *) &timeout_event) {
        dispatcher_wait_event(&dispatcher, &event);
    }
    ASSERT_EQ(event->error_number, ECANCELED);
    event = nullptr;
    while (event != (struct dispatcher_event *) &timeout_event) {
        dispatcher_wait_event(&dispatcher, &event);
    }
    ASSERT_NE(event->error_number, ECANCELED);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = timespec_to_double(end) - timespec_to_double(start);
    ASSERT_TRUE(elapsed >= timeout_ns / 1e9);
    ASSERT_TRUE(dispatcher_destroy(&dispatcher));
}
