#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "session_options.h"
#include "../utils/io.h"

static bool parse_mode(enum tftp_mode *mode, const char mode_str[static 1]);

bool session_options_init(struct session_options session_options[static 1],
                          size_t n,
                          const char options[static n],
                          const char *path[static 1],
                          enum tftp_mode mode[static 1],
                          uint8_t timeout[static 1],
                          uint16_t block_size[static 1],
                          uint16_t window_size[static 1],
                          bool adaptive_timeout[static 1],
                          struct tftp_session_stats_error error[static 1]) {
    *session_options = (struct session_options) {
        .path = path,
        .mode = mode,
        .timeout = timeout,
        .block_size = block_size,
        .window_size = window_size,
        .adaptive_timeout = adaptive_timeout,
    };
    memcpy(session_options->options_storage, options, n);
    *path = &session_options->options_storage[0];
    session_options->mode_str = &session_options->options_storage[strlen(*path) + 1];
    session_options->options_str_size = n - (strlen(*path) + 1 + strlen(session_options->mode_str) + 1);
    session_options->options_str = session_options->options_str_size == 0 ? nullptr : &session_options->options_storage[n - session_options->options_str_size];
    if (!parse_mode(mode, session_options->mode_str)) {
        *error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_ILLEGAL_OPERATION,
            .error_message = "Unknown mode.",
        };
        return false;
    }
    return true;
}

static bool parse_mode(enum tftp_mode *mode, const char mode_str[static 1]) {
    if (strcasecmp(mode_str, "octet") == 0) {
        *mode = TFTP_MODE_OCTET;
        return true;
    }
    if (strcasecmp(mode_str, "netascii") == 0) {
        *mode = TFTP_MODE_NETASCII;
        return true;
    }
    *mode = TFTP_MODE_INVALID;
    return false;
}

bool parse_options(struct session_options options[static 1], int file_descriptor, bool is_adaptive_timeout_enabled) {
    tftp_parse_options(options->recognized_options, options->options_str_size, options->options_str);
    if (is_adaptive_timeout_enabled) {
        const char *option = options->options_str;
        const char *end_ptr = &options->options_str[options->options_str_size - 1];
        while (option < end_ptr) {
            const char *val = &option[strlen(option) + 1];
            if (strcasecmp(option, tftp_option_recognized_string[TFTP_OPTION_TIMEOUT]) == 0 && strcasecmp(val, "adaptive") == 0) {
                *options->adaptive_timeout = true;
                options->recognized_options[TFTP_OPTION_TIMEOUT] = (struct tftp_option) {
                    .is_active = true,
                    .value = "adaptive",
                };
                break;
            }
            val = &option[strlen(option) + 1];
            option = &val[strlen(val) + 1];
        }
    }
    for (enum tftp_option_recognized o = 0; o < TFTP_OPTION_TOTAL_OPTIONS; o++) {
        if (options->recognized_options[o].is_active) {
            options->valid_options_required = true;
            switch (o) {
                case TFTP_OPTION_BLKSIZE:
                    *options->block_size = strtoul(options->recognized_options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_TIMEOUT:
                    if (strcasecmp(options->recognized_options[o].value, "adaptive") == 0) {
                        *options->adaptive_timeout = true;
                    }
                    else {
                        *options->timeout = strtoul(options->recognized_options[o].value, nullptr, 10);
                    }
                    break;
                case TFTP_OPTION_TSIZE:
                    size_t size;
                    auto file_size_cb = (*options->mode == TFTP_MODE_OCTET) ? file_size_octet : file_size_netascii;
                    if (!file_size_cb(file_descriptor, &size)) {
                        // File does not support being queried for file size.
                        options->recognized_options[o].is_active = false;
                        break;
                    }
                    sprintf((char *) options->recognized_options[o].value, "%zu", size);
                    break;
                case TFTP_OPTION_WINDOWSIZE:
                    *options->window_size = strtoul(options->recognized_options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_READ_TYPE:
                    break;
                default:
                    unreachable();
            }
        }
    }
    return true;
}

enum tftp_read_type session_options_get_read_type(struct session_options options[static 1]) {
    tftp_parse_options(options->recognized_options, options->options_str_size, options->options_str);
    struct tftp_option o = options->recognized_options[TFTP_OPTION_READ_TYPE];
    return !o.is_active ? TFTP_READ_TYPE_FILE :
           strcasecmp(o.value, "directory") == 0 ? TFTP_READ_TYPE_DIRECTORY :
                                                   TFTP_READ_TYPE_INVALID;
}
