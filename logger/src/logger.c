#include "logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <sys/syslog.h>
#include <time.h>
#include <threads.h>
#include <unistd.h>
#include <stdlib.h>

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
    
    static const char *logger_level_colors[] = {
        [LOGGER_LOG_LEVEL_FATAL] = COLOR_MAGENTA,
        [LOGGER_LOG_LEVEL_ERROR] = COLOR_RED,
        [LOGGER_LOG_LEVEL_WARN] = COLOR_YELLOW,
        [LOGGER_LOG_LEVEL_INFO] = COLOR_GREEN,
        [LOGGER_LOG_LEVEL_DEBUG] = COLOR_CYAN,
        [LOGGER_LOG_LEVEL_TRACE] = COLOR_LIGHT_BLUE
    };
#endif // LOGGER_USE_COLOR

static const char *logger_levels_str[] = {
    [LOGGER_LOG_LEVEL_FATAL] = "FATAL",
    [LOGGER_LOG_LEVEL_ERROR] = "ERROR",
    [LOGGER_LOG_LEVEL_WARN] = "WARN",
    [LOGGER_LOG_LEVEL_INFO] = "INFO",
    [LOGGER_LOG_LEVEL_DEBUG] = "DEBUG",
    [LOGGER_LOG_LEVEL_TRACE] = "TRACE"
};

static const int logger_levels_syslog[] = {
    [LOGGER_LOG_LEVEL_FATAL] = LOG_CRIT,
    [LOGGER_LOG_LEVEL_ERROR] = LOG_ERR,
    [LOGGER_LOG_LEVEL_WARN] = LOG_WARNING,
    [LOGGER_LOG_LEVEL_INFO] = LOG_INFO,
    [LOGGER_LOG_LEVEL_DEBUG] = LOG_DEBUG,
    [LOGGER_LOG_LEVEL_TRACE] = LOG_DEBUG
};

/**
 * Static variables and functions required to use different ident thread safely.
 */
static mtx_t syslog_mutex;
static const char *syslog_current_ident = nullptr;

[[maybe_unused]]
[[gnu::constructor]]
static void syslog_mutex_init() {
    if (mtx_init(&syslog_mutex, mtx_plain) != thrd_success) {
        exit(EXIT_FAILURE);
    }
}

[[maybe_unused]]
[[gnu::destructor]]
static void syslog_mutex_destroy() {
    mtx_destroy(&syslog_mutex);
}

static inline char *get_date_time(char(*date_time)[20]) {
    time_t t = time(nullptr);
    struct tm *tm = localtime(&t);
    strftime(*date_time, sizeof *date_time, "%F %T", tm);
    return *date_time;
}

extern bool logger_init(struct logger logger[static 1], struct logger_config config) {
    logger->config = config;
    if (!logger->config.use_syslog && logger->config.file == nullptr) {
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
                       const char *fmt, ...) {
    if (level > logger->config.default_level) {
        return;
    }
    if (mtx_lock(&logger->mutex) == thrd_error) {
        return;
    }
    
    va_list args;
    va_start(args, fmt);
    if (logger->config.use_syslog) {
        mtx_lock(&syslog_mutex);
        if (syslog_current_ident != logger->config.syslog_ident) {
            closelog();
            openlog(logger->config.syslog_ident, LOG_PID, LOG_DAEMON);
            syslog_current_ident = logger->config.syslog_ident;
        }
        vsyslog(LOG_DAEMON | logger_levels_syslog[level], fmt, args);
        mtx_unlock(&syslog_mutex);
    }
    else {
        if (logger->config.show_date_time) {
            fprintf(logger->config.file, "%s ", get_date_time(&(char[20]) {}));
        }
        if (logger->config.show_level_name) {
#ifdef LOGGER_USE_COLOR
            fprintf(logger->config.file, "%s%-5s" RESET_STYLE " ", logger_level_colors[level], logger_levels_str[level]);
#else
            log(logger, "%-5s ", logger_levels_str[level]);
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
        vfprintf(logger->config.file, fmt, args);
        fprintf(logger->config.file, "\n");
        fflush(logger->config.file);
    }
    va_end(args);
    while (mtx_unlock(&logger->mutex) != thrd_success);
}
