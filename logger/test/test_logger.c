#include <buracchi/cutest/cutest.h>

#include "logger.h"

TEST(logger, dummy) {
    struct logger logger;
    logger_init(&logger, logger_default_config);
    logger.config.default_level = LOGGER_LOG_LEVEL_ALL;
    logger_log_fatal(&logger, "Message");
    logger_log_error(&logger, "Message");
    logger_log_warn(&logger, "Message");
    logger_log_info(&logger, "Message");
    logger_log_debug(&logger, "Message");
    logger_log_trace(&logger, "Message");
    logger_destroy(&logger);
    ASSERT_EQ(0, 0);
}
