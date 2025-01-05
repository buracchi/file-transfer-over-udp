# ifndef MOCK_LOGGER_H
# define MOCK_LOGGER_H

#include <logger.h>

// NOLINTBEGIN(*-reserved-identifier)

bool __real_logger_init(struct logger logger[static 1], struct logger_config config);
bool __wrap_logger_init([[maybe_unused]] struct logger logger[static 1], [[maybe_unused]] struct logger_config config) {
    return true;
}

void __real_logger_destroy(struct logger logger[static 1]);
void __wrap_logger_destroy([[maybe_unused]] struct logger logger[static 1]) {
    // NOP
}

void __real_logger_log(struct logger logger[static 1], enum logger_log_level level, const char *file, int line, const char *fmt, ...);
void __wrap_logger_log([[maybe_unused]] struct logger logger[static 1],
                       [[maybe_unused]] enum logger_log_level level,
                       [[maybe_unused]] const char *file,
                       [[maybe_unused]] int line,
                       [[maybe_unused]] const char *fmt, ...) {
    // NOP
}

// NOLINTEND(*-reserved-identifier)

#endif // MOCK_LOGGER_H
