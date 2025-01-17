#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <string.h>

#define strerror_max_size 64
#define strerror_rbs(error_code) strerror_r_no_fail(error_code, (char[strerror_max_size]){})
#define strerror_err _Generic((strerror_r(0, &(char){}, 0)), char *: nullptr, int: 0) // XSI-compliant vs GNU-specific

static inline char* strerror_r_no_fail(int error_code, char buffer[static strerror_max_size]) {
    if (strerror_r(error_code, buffer, strerror_max_size) != strerror_err) {
        snprintf(buffer, strerror_max_size, "strerror_r failed, error code is %d.", error_code);
    }
    return buffer;
}

#endif // UTILS_H
