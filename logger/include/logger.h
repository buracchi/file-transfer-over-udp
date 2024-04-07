#pragma once

#include <stdio.h>
#include <stdint.h>
#include <threads.h>

enum logger_log_level : uint8_t {
    LOGGER_LOG_LEVEL_OFF = 0,           // No events will be logged.
    LOGGER_LOG_LEVEL_FATAL = 1,         // A fatal event that will prevent the application from continuing.
    LOGGER_LOG_LEVEL_ERROR = 2,         // An error in the application, possibly recoverable.
    LOGGER_LOG_LEVEL_WARN = 3,          // An event that might possible lead to an error.
    LOGGER_LOG_LEVEL_INFO = 4,          // An event for informational purposes.
    LOGGER_LOG_LEVEL_DEBUG = 5,         // A general debugging event.
    LOGGER_LOG_LEVEL_TRACE = 6,         // A fine-grained debug message, typically capturing the flow through the application.
    LOGGER_LOG_LEVEL_ALL = UINT8_MAX    // All events should be logged.
};

struct logger_config {
    enum logger_log_level default_level;
    FILE *file;
    bool show_date_time;
    bool show_source_file;
    bool show_process_id;
    bool show_thread_id;
    bool show_level_name;
};

struct logger {
    struct logger_config config;
    mtx_t mutex;
};

static const struct logger_config logger_default_config = {
    .default_level = LOGGER_LOG_LEVEL_INFO,
    .file = nullptr,
    .show_date_time = true,
    .show_source_file = false,
    .show_process_id = false,
    .show_thread_id = false,
    .show_level_name = true
};

#define logger_log_trace(logger, ...) logger_log(logger, LOGGER_LOG_LEVEL_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define logger_log_debug(logger, ...) logger_log(logger, LOGGER_LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define logger_log_info(logger, ...)  logger_log(logger, LOGGER_LOG_LEVEL_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define logger_log_warn(logger, ...)  logger_log(logger, LOGGER_LOG_LEVEL_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define logger_log_error(logger, ...) logger_log(logger, LOGGER_LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define logger_log_fatal(logger, ...) logger_log(logger, LOGGER_LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/**
 * If config.file is nullptr, the logger will be initialized to log to stderr.
 */
extern bool logger_init(struct logger logger[static 1], struct logger_config config);

extern void logger_destroy(struct logger logger[static 1]);

extern void logger_log(struct logger logger[static 1], enum logger_log_level level, const char *file, int line, const char *fmt, ...);
