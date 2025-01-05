#include "session_handler.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <buracchi/tftp/server_session_stats.h>

#include <logger.h>
#include <tftp.h>

static bool required_file_setup(struct tftp_handler handler[static 1], const char *root);
static bool parse_options(struct tftp_handler handler[static 1], bool is_adaptive_timeout_enabled);
static bool parse_mode(struct tftp_handler handler[static 1], const char mode[static 1]);

static bool file_size_octet(int fd, size_t size[static 1]);
static bool file_size_netascii(int fd, size_t size[static 1]);

/** if n > tftp_request_packet_max_size - sizeof(enum tftp_opcode) behaviour is undefined **/
// TODO: Reduce cognitive complexity
bool tftp_handler_init(struct tftp_handler handler[static 1],
                       const char *root,
                       size_t n,
                       const char options[static n],
                       uint8_t retries,
                       uint8_t timeout,
                       bool is_adaptive_timeout_enabled,
                       struct tftp_session_stats stats[static 1],
                       struct logger logger[static 1]) {
    *handler = (struct tftp_handler) {
        .logger = logger,
        .data_packets = nullptr,
        .retries = retries,
        .timeout = timeout,
        .block_size = tftp_default_blksize,
        .window_size = 1,
        .file_descriptor = -1,
        .file_iovec = nullptr,
        .next_data_packet_to_send = 1,
        .next_data_packet_to_make = 1,
        .netascii_buffer = -1,
        .stats = stats,
    };

    memcpy(handler->options_storage, options, n);
    handler->path = &handler->options_storage[0];
    const char *mode = &handler->options_storage[strlen(handler->path) + 1];
    handler->options_str_size = n - (strlen(handler->path) + 1 + strlen(mode) + 1);
    handler->options_str = handler->options_str_size == 0 ? nullptr : &handler->options_storage[n - handler->options_str_size];
    
    if (!parse_mode(handler, mode)) {
        return true;
    }
    if (!required_file_setup(handler, root)) {
        return true;
    }
    if (!parse_options(handler, is_adaptive_timeout_enabled)) {
        return true;
    }
    handler->data_packets = malloc(handler->window_size * (sizeof *handler->data_packets + handler->block_size));
    if (handler->data_packets == nullptr) {
        logger_log_error(logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    handler->file_iovec = malloc(handler->window_size * sizeof *handler->file_iovec);
    if (handler->file_iovec == nullptr) {
        logger_log_error(logger, "Not enough memory: %s", strerror(errno));
        return false;
    }
    return true;
}

void tftp_handler_destroy(struct tftp_handler handler[static 1]) {
    handler->stats->retransmits = handler->global_retransmits;
    if (handler->stats->callback != nullptr) {
        handler->stats->callback(handler->stats);
    }
    logger_log_debug(handler->logger, "Closing response data object.");
    close(handler->file_descriptor);
    free(handler->data_packets);
    free(handler->file_iovec);
}

void next_block_setup(struct tftp_handler handler[static 1]) {
    uint16_t block_diff = handler->next_data_packet_to_make - handler->window_begin;
    size_t available_packets = handler->window_size - block_diff;
    for (size_t i = 0; i < available_packets; i++) {
        size_t next_data_packet_index = (handler->next_data_packet_to_make - 1 + i) % handler->window_size;
        size_t offset = next_data_packet_index * (sizeof(struct tftp_data_packet) + handler->block_size);
        struct tftp_data_packet *packet = (void *) ((uint8_t *) handler->data_packets + offset);
        handler->file_iovec[i].iov_base = packet->data;
        handler->file_iovec[i].iov_len = handler->block_size;
    }
    // handle partial reads
    handler->last_block_size = handler->incomplete_read ? handler->last_block_size : 0;
    handler->incomplete_read = false;
    handler->file_iovec[0].iov_base = &((uint8_t *) handler->file_iovec[0].iov_base)[handler->last_block_size];
    handler->file_iovec[0].iov_len -= handler->last_block_size;
}

static bool parse_mode(struct tftp_handler handler[static 1], const char mode[static 1]) {
    if (strcasecmp(mode, "octet") == 0) {
        handler->mode = TFTP_MODE_OCTET;
    }
    else if (strcasecmp(mode, "netascii") == 0) {
        handler->mode = TFTP_MODE_NETASCII;
    }
    else {
        handler->mode = TFTP_MODE_INVALID;
        const char format[] = "Unknown mode: '%s'.";
        char *message = malloc(sizeof format + strlen(mode));
        if (message == nullptr) {
            // TODO: handle this better
            logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
            return false;
        }
        sprintf(message, format, mode);
        handler->stats->error = (struct tftp_session_stats_error) {
                .error_occurred = true,
                .error_number = TFTP_ERROR_ILLEGAL_OPERATION,
                .error_message = message,
        };
    }
    handler->stats->mode = tftp_mode_to_string(handler->mode);
    return (handler->mode != TFTP_MODE_INVALID);
}

static bool parse_options(struct tftp_handler handler[static 1], bool is_adaptive_timeout_enabled) {
    if (handler->options_str == nullptr) {
        logger_log_info(handler->logger, "No options requested from peer %s:%d.", handler->stats->peer_addr, handler->stats->peer_port);
        return true;
    }
    tftp_format_option_strings(handler->options_str_size, handler->options_str, handler->stats->options_in);
    logger_log_info(handler->logger, "Options requested from peer %s:%d are [%s]", handler->stats->peer_addr, handler->stats->peer_port, handler->stats->options_in);
    tftp_parse_options(handler->options, handler->options_str_size, handler->options_str);
    if (is_adaptive_timeout_enabled) {
        const char *option = handler->options_str;
        const char *end_ptr = &handler->options_str[handler->options_str_size - 1];
        while (option < end_ptr) {
            const char *val = &option[strlen(option) + 1];
            if (strcasecmp(option, tftp_option_recognized_string[TFTP_OPTION_TIMEOUT]) == 0
                && strcasecmp(val, "adaptive") == 0) {
                handler->adaptive_timeout = true;
                handler->options[TFTP_OPTION_TIMEOUT] = (struct tftp_option) {
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
        if (handler->options[o].is_active) {
            handler->valid_options_required = true;
            switch (o) {
                case TFTP_OPTION_BLKSIZE:
                    handler->block_size = strtoul(handler->options[o].value, nullptr, 10);
                    break;
                case TFTP_OPTION_TIMEOUT:
                    if (strcasecmp(handler->options[o].value, "adaptive") == 0) {
                        handler->adaptive_timeout = true;
                    }
                    else {
                        handler->timeout = strtoul(handler->options[o].value, nullptr, 10);
                    }
                    break;
                case TFTP_OPTION_TSIZE:
                    size_t size;
                    bool (*file_size)(int, size_t *) = (handler->mode == TFTP_MODE_OCTET) ?
                                                       file_size_octet :
                                                       file_size_netascii;
                    if (!file_size(handler->file_descriptor, &size)) {
                        handler->stats->error = (struct tftp_session_stats_error) {
                            .error_occurred = true,
                            .error_number = TFTP_ERROR_INVALID_OPTIONS,
                            .error_message = "File does not support being queried for file size."
                        };
                        return false;
                    }
                    sprintf((char *) handler->options[o].value, "%zu", size);
                    break;
                case TFTP_OPTION_WINDOWSIZE:
                    handler->window_size = strtoul(handler->options[o].value, nullptr, 10);
                    break;
                default: // unreachable
                    break;
            }
        }
    }
    tftp_format_options(handler->options, handler->stats->options_acked);
    logger_log_info(handler->logger, "Options to ack for peer %s:%d are %s", handler->stats->peer_addr, handler->stats->peer_port, handler->stats->options_acked);
    handler->stats->blksize = handler->block_size;
    handler->stats->window_size = handler->window_size;
    return true;
}

static bool required_file_setup(struct tftp_handler handler[static 1], const char *root) {
    const char *path;
    if (root == nullptr) {
        path = handler->path;
    }
    else {
        bool is_root_slash_terminated = root[strlen(root) - 1] == '/';
        size_t padding = is_root_slash_terminated ? 0 : 1;
        char *full_path = malloc(strlen(root) + strlen(handler->path) + padding + 1);
        if (full_path == nullptr) {
            // TODO: handle this better
            logger_log_error(handler->logger, "Not enough memory: %s", strerror(errno));
            return false;
        }
        strcpy(full_path, root);
        if (!is_root_slash_terminated) {
            strcat(full_path, "/");
        }
        strcat(full_path, handler->path);
        path = full_path;
    }
    errno = 0;
    do {
        handler->file_descriptor = open(path, O_RDONLY);
    } while (handler->file_descriptor == -1 && errno == EINTR);
    if (root != nullptr) {
        free((void *) path);
    }
    if (handler->file_descriptor == -1) {
        enum tftp_error_code error_code;
        char *message;
        switch (errno) {
            case EACCES:
                error_code = TFTP_ERROR_ACCESS_VIOLATION;
                message = "Permission denied.";
                break;
            case ENOENT:
                error_code = TFTP_ERROR_FILE_NOT_FOUND;
                message = "No such file or directory.";
                break;
            default:
                error_code = TFTP_ERROR_NOT_DEFINED;
                message = strerror(errno);
        }
        handler->stats->error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = error_code,
            .error_message = message,
        };
        return false;
    }
    return true;
}

static bool file_size_octet(int fd, size_t size[static 1]) {
    struct stat file_stat;
    if (fd < 0) {
        return false;
    }
    if (fstat(fd, &file_stat) < 0) {
        return false;
    }
    switch (file_stat.st_mode & __S_IFMT) {
        case __S_IFBLK:
            uint64_t bytes;
            if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
                return false;
            }
            *size = bytes;
            return true;
        case __S_IFREG:
            *size = file_stat.st_size;
            return true;
        default:
            return false;
    }
}

static bool file_size_netascii(int fd, size_t size[static 1]) {
    size_t netascii_size = 0;
    size_t bytes_to_read;
    if (!file_size_octet(fd, &bytes_to_read)) {
        return false;
    }
    size_t buffer_size = 4096;
    char *buffer = malloc(buffer_size);
    if (buffer == nullptr) {
        return false;
    }
    while (bytes_to_read) {
        ssize_t bytes_read = read(fd, buffer, buffer_size);
        if (bytes_read == -1) {
            free(buffer);
            return false;
        }
        netascii_size += bytes_read;
        for (size_t i = 0; i < buffer_size; i++) {
            if (buffer[i] == '\n' || buffer[i] == '\r') {
                ++netascii_size;
            }
        }
        bytes_to_read -= bytes_read;
    }
    free(buffer);
    if (lseek(fd, 0, SEEK_SET) == -1) {
        return false;
    }
    *size = netascii_size;
    return true;
}
