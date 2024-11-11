#include <buracchi/cutest/cutest.h>

#include <buracchi/tftp/dispatcher.h>

TEST(dispatcher, register_and_dispatch_single_event) {
    struct logger logger;
    struct dispatcher dispatcher;
    struct dispatcher_event event;
    logger_init(&logger, logger_default_config);
    ASSERT_TRUE(dispatcher_init(&dispatcher, 32, &logger));
    ASSERT_EQ(dispatcher.pending_requests, 0);
    ASSERT_TRUE(dispatcher_submit(&dispatcher, &event));
    ASSERT_EQ(1, dispatcher.pending_requests);
    ASSERT_TRUE(dispatcher_destroy(&dispatcher));
}
