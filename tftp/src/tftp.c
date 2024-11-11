#include <tftp.h>

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netinet/in.h>

/**
 * While being approved for C23, as of the time of writing, N3022 is still unsupported by any compiler I know.
 * The following code is a workaround for the lack of support of the <stdbit.h> header.
 */
#include <endian.h>

#ifndef TFTP_H16TOBE16
#   if __BYTE_ORDER == __LITTLE_ENDIAN
#       define TFTP_H16TOBE16(n) ((((n) >> 8) & 0xFFu) | (((n) << 8) & 0xFF00u))
#   elif __BYTE_ORDER == __BIG_ENDIAN
#       define define TFTP_H16TOBE16(n) (n)
#   else
#       error "Unsupported architecture. Define a TFTP_H16TOBE16 macro for your architecture in order to use this library."
#   endif
#endif

#define TFTP_ERROR_NOT_DEFINED_MESSAGE "Not defined, see error message (if any)."
#define TFTP_ERROR_FILE_NOT_FOUND_MESSAGE "File not found."
#define TFTP_ERROR_ACCESS_VIOLATION_MESSAGE "Access violation."
#define TFTP_ERROR_DISK_FULL_MESSAGE "Disk full or allocation exceeded."
#define TFTP_ERROR_ILLEGAL_OPERATION_MESSAGE "Illegal TFTP operation."
#define TFTP_ERROR_UNKNOWN_TRANSFER_ID_MESSAGE "Unknown transfer ID."
#define TFTP_ERROR_FILE_ALREADY_EXISTS_MESSAGE "File already exists."
#define TFTP_ERROR_NO_SUCH_USER_MESSAGE "No such user."
#define TFTP_ERROR_INVALID_OPTIONS_MESSAGE "Invalid options."

#define TFTP_ERROR_PACKET_INFO_INIT(err_code) {                             \
    .packet = (struct tftp_error_packet*)&(struct {                         \
            const enum tftp_opcode opcode;                                  \
            const enum tftp_error_code error_code;                          \
            const uint8_t error_message[sizeof(err_code##_MESSAGE)];        \
            }) {                                                            \
        .opcode = TFTP_H16TOBE16(TFTP_OPCODE_ERROR),                        \
        .error_code = TFTP_H16TOBE16(err_code),                             \
        .error_message = err_code##_MESSAGE                                 \
        },                                                                  \
    .size = sizeof(struct tftp_error_packet) + sizeof(err_code##_MESSAGE),  \
}

const struct tftp_error_packet_info tftp_error_packet_info[] = {
    [TFTP_ERROR_NOT_DEFINED] = {.packet = nullptr, .size = 0},
    [TFTP_ERROR_FILE_NOT_FOUND] = TFTP_ERROR_PACKET_INFO_INIT(TFTP_ERROR_FILE_NOT_FOUND),
    [TFTP_ERROR_ACCESS_VIOLATION] = TFTP_ERROR_PACKET_INFO_INIT(TFTP_ERROR_ACCESS_VIOLATION),
    [TFTP_ERROR_DISK_FULL] = TFTP_ERROR_PACKET_INFO_INIT(TFTP_ERROR_DISK_FULL),
    [TFTP_ERROR_ILLEGAL_OPERATION] = TFTP_ERROR_PACKET_INFO_INIT(TFTP_ERROR_ILLEGAL_OPERATION),
    [TFTP_ERROR_UNKNOWN_TRANSFER_ID] = TFTP_ERROR_PACKET_INFO_INIT(TFTP_ERROR_UNKNOWN_TRANSFER_ID),
    [TFTP_ERROR_FILE_ALREADY_EXISTS] = TFTP_ERROR_PACKET_INFO_INIT(TFTP_ERROR_FILE_ALREADY_EXISTS),
    [TFTP_ERROR_NO_SUCH_USER] = TFTP_ERROR_PACKET_INFO_INIT(TFTP_ERROR_NO_SUCH_USER),
    [TFTP_ERROR_INVALID_OPTIONS] = TFTP_ERROR_PACKET_INFO_INIT(TFTP_ERROR_INVALID_OPTIONS),
};

static const char *tftp_mode_str[] = {
    [TFTP_MODE_OCTET] = "octet",
    [TFTP_MODE_NETASCII] = "netascii",
    [TFTP_MODE_INVALID] = "invalid"
};

extern const char *tftp_mode_to_string(enum tftp_mode mode) {
    return tftp_mode_str[mode];
}

static enum tftp_mode tftp_get_mode(size_t n, const char mode[static n]);

const char *tftp_option_recognized_string[] = {
        [TFTP_OPTION_BLKSIZE] = "blksize",
        [TFTP_OPTION_TIMEOUT] = "timeout",
        [TFTP_OPTION_TSIZE] = "tsize",
        [TFTP_OPTION_WINDOWSIZE] = "windowsize"
};

enum tftp_opcode tftp_get_opcode_unsafe(const void *packet) {
    enum tftp_opcode opcode;
    // We use memcpy since loading after casting to (uint16_t *) without 2 byte alignment is UB
    memcpy(&opcode, packet, sizeof opcode);
    opcode = ntohs(opcode);
    return opcode;
}

void tftp_data_packet_init(struct tftp_data_packet *packet, uint16_t block_number) {
    *packet = (struct tftp_data_packet) {
        .opcode = htons(TFTP_OPCODE_DATA),
        .block_number = htons(block_number)
    };
}

void tftp_ack_packet_init(struct tftp_ack_packet packet[static 1], uint16_t block_number) {
    *packet = (struct tftp_ack_packet) {
        .opcode = htons(TFTP_OPCODE_ACK),
        .block_number = htons(block_number)
    };
}

ssize_t tftp_rrq_packet_init(struct tftp_rrq_packet packet[static 1], size_t n, const char filename[static n], enum tftp_mode mode, struct tftp_option options[static TFTP_OPTION_TOTAL_OPTIONS]) {
    packet->opcode = htons(TFTP_OPCODE_RRQ);
    size_t filename_len = strnlen(filename, n);
    size_t mode_len = strlen(tftp_mode_str[mode]);
    if (filename_len == n || (filename_len + 1 + mode_len + 1) > sizeof packet->padding) {
        return -1;
    }
    uint8_t *mode_ptr = memccpy(packet->padding, filename, '\0', n);
    uint8_t *end_ptr = memccpy(mode_ptr, tftp_mode_str[mode], '\0', mode_len + 1);
    for (enum tftp_option_recognized o = 0; o < TFTP_OPTION_TOTAL_OPTIONS; o++) {
        if (options[o].is_active) {
            size_t option_len = strlen(tftp_option_recognized_string[o]);
            size_t value_len = strlen(options[o].value);
            if ((option_len + 1 + value_len + 1) > (sizeof packet->padding - (end_ptr - packet->padding))) {
                return -1;
            }
            end_ptr = memccpy(end_ptr, tftp_option_recognized_string[o], '\0', strlen(tftp_option_recognized_string[o]) + 1);
            end_ptr = memccpy(end_ptr, options[o].value, '\0', strlen(options[o].value) + 1);
        }
    }
    return (ssize_t) sizeof packet->opcode + (end_ptr - packet->padding);
}

extern enum tftp_create_rqp_error tftp_encode_request(struct tftp_packet packet[static 1], size_t psize[static 1], enum tftp_request_type type, size_t n, const char filename[static n], enum tftp_mode mode) {
    if (mode != TFTP_MODE_OCTET && mode != TFTP_MODE_NETASCII) {
        return TFTP_CREATE_RQP_INVALID_MODE;
    }
    size_t filename_len = strnlen(filename, n);
    size_t mode_len = strlen(tftp_mode_str[mode]);
    if (filename_len == n || (filename_len + 1 + mode_len + 1) > 510) {
        return TFTP_CREATE_RQP_TRUNCATED_FILENAME;
    }
    packet->opcode = htons(type == TFTP_REQUEST_READ ? TFTP_OPCODE_RRQ : TFTP_OPCODE_WRQ);
    uint8_t *mode_ptr = memccpy(type == TFTP_REQUEST_READ ? packet->rrq : packet->wrq, filename, '\0', n);
    uint8_t *end_ptr = memccpy(mode_ptr, tftp_mode_str[mode], '\0', mode_len + 1);
    *psize = end_ptr - (uint8_t *) packet;
    return TFTP_CREATE_RQP_SUCCESS;
}

extern struct tftp_packet tftp_encode_error(enum tftp_error_code error_code, size_t n, char error_message[static n]) {
    struct tftp_packet packet = {
        .opcode = htons(TFTP_OPCODE_ERROR),
        .error = {
            .error_code = htons(error_code),
            .error_message = {0}
        }
    };
    strncpy((char *) packet.error.error_message, error_message, n);
    return packet;
}

extern struct tftp_packet tftp_encode_data(uint16_t block_number, size_t n, const uint8_t data[n],
                                           size_t *packet_size) {
    struct tftp_packet packet = {
        .opcode = htons(TFTP_OPCODE_DATA),
        .data = {
            .block_number = htons(block_number)
        }
    };
    memcpy(packet.data.data, data, n);
    *packet_size = sizeof packet.opcode + sizeof packet.data.block_number + n;
    return packet;
}

extern struct tftp_packet tftp_encode_ack(uint16_t block_number) {
    return (struct tftp_packet) {
        .opcode = htons(TFTP_OPCODE_ACK),
        .ack = {
            .block_number = htons(block_number)
        }
    };
}

extern bool tftp_parse_packet(struct tftp_packet packet[static 1], size_t n, const uint8_t data[static n]) {
    if (n < 4) {
        // Packet is too small to contain an opcode and any meaningful field
        return false;
    }
    enum tftp_opcode opcode = tftp_get_opcode_unsafe(data);
    const uint8_t *data_to_parse = data + sizeof packet->opcode;
    size_t data_to_parse_size = n - sizeof packet->opcode;
    switch (opcode) {
        case TFTP_OPCODE_RRQ:
            packet->opcode = opcode;
            memcpy(packet->rrq, data_to_parse, data_to_parse_size);
            break;
        case TFTP_OPCODE_WRQ:
            packet->opcode = opcode;
            memcpy(packet->wrq, data_to_parse, data_to_parse_size);
            break;
        case TFTP_OPCODE_DATA:
            packet->opcode = opcode;
            packet->data.block_number = ntohs(*(uint16_t *) data_to_parse);
            data_to_parse += sizeof packet->data.block_number;
            data_to_parse_size -= sizeof packet->data.block_number;
            memcpy(packet->data.data, data_to_parse, data_to_parse_size);
            break;
        case TFTP_OPCODE_ACK:
            packet->opcode = opcode;
            packet->ack.block_number = ntohs(*(uint16_t *) data_to_parse);
            break;
        case TFTP_OPCODE_ERROR:
            if (n < 5) {
                return false;
            }
            packet->opcode = opcode;
            packet->error.error_code = ntohs(*(uint16_t *) data_to_parse);
            data_to_parse += sizeof packet->error.error_code;
            data_to_parse_size -= sizeof packet->error.error_code;
            memcpy(packet->error.error_message, data_to_parse, data_to_parse_size);
            break;
    }
    return true;
}

extern struct tftp_request tftp_decode_request(size_t n, const uint8_t data[n]) {
    if (n > tftp_request_packet_max_size) {
        return (struct tftp_request) {.type = TFT_REQUEST_INVALID, .error = TFTP_REQUEST_ERROR_MAX_SIZE_EXCEEDED};
    }
    enum tftp_opcode opcode = ntohs(*(uint16_t *) data);
    if (opcode != TFTP_OPCODE_RRQ && opcode != TFTP_OPCODE_WRQ) {
        return (struct tftp_request) {.type = TFT_REQUEST_INVALID, .error = TFTP_REQUEST_ERROR_NOT_A_REQUEST};
    }
    const uint8_t *data_to_parse = data + sizeof opcode;
    size_t data_to_parse_size = n - sizeof opcode;
    const char *filename_ptr = (const char *) data_to_parse;
    size_t filename_len = strnlen(filename_ptr, data_to_parse_size);
    if (filename_len == 0 || filename_len > (data_to_parse_size - 3)) {    // 2 zero bytes and at least 1 byte for mode
        return (struct tftp_request) {.type = TFT_REQUEST_INVALID, .error = TFTP_REQUEST_ERROR_INVALID_FILENAME};
    }
    data_to_parse += filename_len + 1;
    data_to_parse_size -= filename_len + 1;
    const char *mode_ptr = (const char *) data_to_parse;
    size_t mode_len = strnlen(mode_ptr, data_to_parse_size);
    if (mode_ptr[mode_len] != '\0') {
        return (struct tftp_request) {.type = TFT_REQUEST_INVALID, .error = TFTP_REQUEST_ERROR_INVALID_MODE};
    }
    enum tftp_mode mode = tftp_get_mode(mode_len + 1, mode_ptr);
    if (mode == TFTP_MODE_INVALID) {
        return (struct tftp_request) {.type = TFT_REQUEST_INVALID, .error = TFTP_REQUEST_ERROR_INVALID_MODE};
    }
    return (struct tftp_request) {
        .type = opcode == TFTP_OPCODE_RRQ ? TFTP_REQUEST_READ : TFTP_REQUEST_WRITE,
        .filename = filename_ptr,
        .mode = mode
    };
}

// TODO: This function is locale dependent. Find out if it's a problem.
static enum tftp_mode tftp_get_mode(size_t n, const char mode[static n]) {
    if (strncasecmp(mode, tftp_mode_str[TFTP_MODE_OCTET], n) == 0) {
        return TFTP_MODE_OCTET;
    }
    if (strncasecmp(mode, tftp_mode_str[TFTP_MODE_NETASCII], n) == 0) {
        return TFTP_MODE_NETASCII;
    }
    return TFTP_MODE_INVALID;
}

void tftp_format_option_strings(size_t n, const char options[static n], char formatted_options[static tftp_option_formatted_string_max_size]) {
    const char *option = options;
    const char *value = option + strlen(option) + 1;
    while (option < options + n) {
        formatted_options += sprintf(formatted_options, "%s:%s", option, value);
        option = value + strlen(value) + 1;
        value = option + strlen(option) + 1;
        if (option < options + n) {
            formatted_options += sprintf(formatted_options, ", ");
        }
    }
}

void tftp_format_options(struct tftp_option options[static TFTP_OPTION_TOTAL_OPTIONS], char formatted_options[static tftp_option_formatted_string_max_size]) {
    formatted_options += sprintf(formatted_options, "[");
    for (enum tftp_option_recognized o = 0; o < TFTP_OPTION_TOTAL_OPTIONS; o++) {
        if (options[o].is_active) {
            formatted_options += sprintf(formatted_options, "%s:%s", tftp_option_recognized_string[o], options[o].value);
            for (enum tftp_option_recognized o_peek = o + 1; o_peek < TFTP_OPTION_TOTAL_OPTIONS; o_peek++) {
                if (options[o_peek].is_active) {
                    formatted_options += sprintf(formatted_options, ", ");
                    break;
                }
            }
        }
    }
    sprintf(formatted_options, "]");
}

void tftp_parse_options(struct tftp_option options[static TFTP_OPTION_TOTAL_OPTIONS], size_t n, const char str[static n]) {
    const char *option = str;
    const char *end_ptr = &str[n - 1];
    while (option < end_ptr) {
        const char *val = &option[strlen(option) + 1];
        for (enum tftp_option_recognized o = 0; o < TFTP_OPTION_TOTAL_OPTIONS; o++) {
            if (strcasecmp(option, tftp_option_recognized_string[o]) == 0) {
                bool is_val_valid = true;
                switch (o) {
                    case TFTP_OPTION_BLKSIZE: {
                        char *not_parsed;
                        errno = 0;
                        size_t blksize = strtoul(val, &not_parsed, 10);
                        if (*val == '\0' || *not_parsed != '\0' || blksize < 8 || blksize > 65464) {
                            is_val_valid = false;
                            break;
                        }
                        while (isspace(*val) || *val == '+') {
                            val++;
                        }
                        break;
                    }
                    case TFTP_OPTION_TIMEOUT: {
                        char *not_parsed;
                        errno = 0;
                        unsigned long timeout = strtoul(val, &not_parsed, 10);
                        if (*val == '\0' || *not_parsed != '\0' || timeout < 1 || timeout > 255) {
                            is_val_valid = false;
                            break;
                        }
                        while (isspace(*val) || *val == '+') {
                            val++;
                        }
                        break;
                    }
                    case TFTP_OPTION_TSIZE: {
                        if (strcmp(val, "0") == 0) {
                            val = calloc(1, 3 * sizeof(size_t) + 1);    // TODO remove malloc
                            break;
                        }
                        char *not_parsed;
                        errno = 0;
                        unsigned long long size = strtoull(val, &not_parsed, 10);
                        if (*val == '\0' || *not_parsed != '\0' || (size == ULLONG_MAX && errno == ERANGE)) {
                            is_val_valid = false;
                            break;
                        }
                        while (isspace(*val) || *val == '+') {
                            val++;
                        }
                        break;
                    }
                    case TFTP_OPTION_WINDOWSIZE: {
                        char *not_parsed;
                        errno = 0;
                        unsigned long window_size = strtoul(val, &not_parsed, 10);
                        if (*val == '\0' || *not_parsed != '\0' || window_size < 1 || window_size > 65535) {
                            is_val_valid = false;
                            break;
                        }
                        while (isspace(*val) || *val == '+') {
                            val++;
                        }
                        break;
                    }
                    default: // unreachable
                        break;
                }
                if (!is_val_valid) {
                    break;
                }
                options[o] = (struct tftp_option) {
                    .is_active = true,
                    .value = val,
                };
                break;
            }
        }
        val = &option[strlen(option) + 1];  // TODO: remove after removing the malloc
        option = &val[strlen(val) + 1];
    }
}
