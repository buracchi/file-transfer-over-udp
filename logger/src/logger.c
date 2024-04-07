#include "logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

#include <unistd.h>
#include <threads.h>

static const char* logger_levels_str[] = {
    [LOGGER_LOG_LEVEL_FATAL] = "FATAL",
    [LOGGER_LOG_LEVEL_ERROR] = "ERROR",
    [LOGGER_LOG_LEVEL_WARN] = "WARN",
    [LOGGER_LOG_LEVEL_INFO] = "INFO",
    [LOGGER_LOG_LEVEL_DEBUG] = "DEBUG",
    [LOGGER_LOG_LEVEL_TRACE] = "TRACE"
};

#define LOGGER_USE_COLOR

#ifdef LOGGER_USE_COLOR

#define RESET_STYLE			"\x1b[0m"

#define COLOR_BLACK			"\x1b[30m"
#define COLOR_RED			"\x1b[31m"
#define COLOR_GREEN			"\x1b[32m"
#define COLOR_YELLOW		"\x1b[33m"
#define COLOR_BLUE			"\x1b[34m"
#define COLOR_MAGENTA		"\x1b[35m"
#define COLOR_CYAN			"\x1b[36m"
#define COLOR_WHITE			"\x1b[37m"

#define COLOR_LIGHT_BLUE	"\x1b[94m"

static const char* logger_level_colors[] = {
    [LOGGER_LOG_LEVEL_FATAL] = COLOR_MAGENTA,
    [LOGGER_LOG_LEVEL_ERROR] = COLOR_RED,
    [LOGGER_LOG_LEVEL_WARN] = COLOR_YELLOW,
    [LOGGER_LOG_LEVEL_INFO] = COLOR_GREEN,
    [LOGGER_LOG_LEVEL_DEBUG] = COLOR_CYAN,
    [LOGGER_LOG_LEVEL_TRACE] = COLOR_LIGHT_BLUE
};
#endif

static inline char* get_date_time(char(*date_time)[20]);

extern bool logger_init(struct logger logger[static 1], struct logger_config config) {
    logger->config = config;
    if (logger->config.file == nullptr) {
        logger->config.file = stderr;
    }
    return mtx_init(&logger->mutex, mtx_plain) == thrd_success;
}

extern void logger_destroy(struct logger logger[static 1]) {
    mtx_destroy(&logger->mutex);
}

extern void logger_log(struct logger logger[static 1],
                       enum logger_log_level level,
                       const char *file,
                       int line,
                       const char *fmt,
                       ...) {
    va_list ap;
    if (logger->config.default_level >= level) {
        if (mtx_lock(&logger->mutex) == thrd_error) {
            return;
        }
        va_start(ap, fmt);
        if (logger->config.show_date_time) {
            fprintf(logger->config.file, "%s ", get_date_time(&(char[20]) {}));
        }
        if (logger->config.show_level_name) {
#ifdef LOGGER_USE_COLOR
            fprintf(logger->config.file, "%s%-5s" RESET_STYLE " ", logger_level_colors[level], logger_levels_str[level]);
#else
            fprintf(logger->config.file, "%-5s ", logger_levels_str[level]);
#endif
        }
        if (logger->config.show_process_id) {
            fprintf(logger->config.file, "%d ", getpid());
        }
        if (logger->config.show_thread_id) {
            fprintf(logger->config.file, "%lu ", thrd_current());
        }
        if (logger->config.show_source_file) {
            fprintf(logger->config.file, "%s: %d ", file, line);
        }
        fprintf(logger->config.file, "- ");
        vfprintf(logger->config.file, fmt, ap);
        fprintf(logger->config.file, "\n");
        while (mtx_unlock(&logger->mutex) != thrd_success);
        fflush(logger->config.file);
        va_end(ap);
    }
}

static inline char* get_date_time(char(*date_time)[20]) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    strftime(*date_time, sizeof * date_time, "%F %T", tm);
    return *date_time;
}
