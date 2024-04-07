#pragma once

#include <stdint.h>
#include <stddef.h>
#include <sys/types.h>

constexpr size_t tftp_request_packet_max_size = 512;
constexpr uint16_t tftp_default_blksize = 512;
constexpr uint16_t tftp_default_window_size = 1;

enum tftp_mode {
    TFTP_MODE_OCTET,
    TFTP_MODE_NETASCII,
    TFTP_MODE_INVALID
};

enum tftp_opcode : uint16_t {
    TFTP_OPCODE_RRQ = 0x01,
    TFTP_OPCODE_WRQ = 0x02,
    TFTP_OPCODE_DATA = 0x03,
    TFTP_OPCODE_ACK = 0x04,
    TFTP_OPCODE_ERROR = 0x05,
    TFTP_OPCODE_OACK = 0x06,
};

enum tftp_error_code : uint16_t {
    TFTP_ERROR_NOT_DEFINED = 0x00,
    TFTP_ERROR_FILE_NOT_FOUND = 0x01,
    TFTP_ERROR_ACCESS_VIOLATION = 0x02,
    TFTP_ERROR_DISK_FULL = 0x03,
    TFTP_ERROR_ILLEGAL_OPERATION = 0x04,
    TFTP_ERROR_UNKNOWN_TRANSFER_ID = 0x05,
    TFTP_ERROR_FILE_ALREADY_EXISTS = 0x06,
    TFTP_ERROR_NO_SUCH_USER = 0x07,
    TFTP_ERROR_INVALID_OPTIONS = 0x08,
};

struct [[gnu::packed]] tftp_packet {
enum tftp_opcode opcode;
union {
    uint8_t rrq[510];
    uint8_t wrq[510];
    struct [[gnu::packed]] {
        uint16_t block_number;
        uint8_t data[512];
    } data;
    struct [[gnu::packed]] {
        uint16_t block_number;
    } ack;
    struct [[gnu::packed]] {
        uint16_t error_code;
        uint8_t error_message[512];
    } error;
};
};

struct [[gnu::packed]] tftp_rrq_packet {
    enum tftp_opcode opcode;
    uint8_t padding[510];
};

struct [[gnu::packed]] tftp_ack_packet {
    enum tftp_opcode opcode;
    uint16_t block_number;
};

struct [[gnu::packed]] tftp_data_packet {
    enum tftp_opcode opcode;
    uint16_t block_number;
    uint8_t data[];
};

struct [[gnu::packed]] tftp_oack_packet {
    enum tftp_opcode opcode;
    unsigned char options_values[];
};

struct [[gnu::packed]] tftp_error_packet {
    const enum tftp_opcode opcode;
    const enum tftp_error_code error_code;
    const uint8_t error_message[];
};

struct tftp_error_packet_info {
    const struct tftp_error_packet *packet;
    size_t size;
};

/**
 * The error packet information for each default error packets.
 * The index of the array is the error code.
 * The error packet information for TFTP_ERROR_NOT_DEFINED is not defined.
 * Use @see tftp_encode_error to send TFTP_ERROR_NOT_DEFINED code
 * packets or to send error packets with custom error messages.
 */
extern const struct tftp_error_packet_info tftp_error_packet_info[];

enum tftp_request_type {
    TFTP_REQUEST_READ,
    TFTP_REQUEST_WRITE,
    TFT_REQUEST_INVALID
};

enum tftp_create_rqp_error {
    TFTP_CREATE_RQP_SUCCESS,
    TFTP_CREATE_RQP_INVALID_MODE,
    TFTP_CREATE_RQP_TRUNCATED_FILENAME,
};

enum tftp_request_error {
    TFTP_REQUEST_ERROR_MAX_SIZE_EXCEEDED,
    TFTP_REQUEST_ERROR_NOT_A_REQUEST,
    TFTP_REQUEST_ERROR_INVALID_FILENAME,
    TFTP_REQUEST_ERROR_INVALID_MODE,
};

struct tftp_request {
    enum tftp_request_type type;
    union {
        struct {
            const char *filename;
            enum tftp_mode mode;
        };
        enum tftp_request_error error;
    };
};

constexpr size_t tftp_option_string_max_size = tftp_request_packet_max_size - sizeof(enum tftp_opcode) - 4;
constexpr size_t tftp_option_formatted_string_max_size =  tftp_option_string_max_size * 2 - 1;

enum tftp_option_recognized {
    TFTP_OPTION_BLKSIZE,
    TFTP_OPTION_TIMEOUT,
    TFTP_OPTION_TSIZE,
    TFTP_OPTION_WINDOWSIZE,
    TFTP_OPTION_TOTAL_OPTIONS
};

extern const char *tftp_option_recognized_string[TFTP_OPTION_TOTAL_OPTIONS];

struct tftp_option {
    bool is_active;
    const char *value;
};

extern enum tftp_create_rqp_error tftp_encode_request(struct tftp_packet packet[1],
                                                      size_t psize[1], enum tftp_request_type type,
                                                      size_t n, const char filename[static n],
                                                      enum tftp_mode mode);

extern bool tftp_parse_packet(struct tftp_packet packet[static 1], size_t n, const uint8_t data[static n]);

extern struct tftp_request tftp_decode_request(size_t n, const uint8_t data[n]);

extern struct tftp_packet tftp_encode_error(enum tftp_error_code error_code, size_t n, char error_message[static n]);

extern struct tftp_packet tftp_encode_data(uint16_t block_number, size_t n, const uint8_t data[n],
                                           size_t *packet_size);

extern struct tftp_packet tftp_encode_ack(uint16_t block_number);

void tftp_format_option_strings(size_t n, const char options[static n], char formatted_options[static tftp_option_formatted_string_max_size]);

void tftp_format_options(struct tftp_option options[4], char formatted_options[static tftp_option_formatted_string_max_size]);

void tftp_parse_options(struct tftp_option options[static TFTP_OPTION_TOTAL_OPTIONS], size_t n, const char str[static n]);

extern const char *tftp_mode_to_string(enum tftp_mode mode);

/**
 * @brief Initialize a TFTP data packet. The data field initialization is left to the caller.
 * If the packet argument points to an invalid memory location, the behavior is undefined.
 */
void tftp_data_packet_init(struct tftp_data_packet *packet, uint16_t block_number);

void tftp_ack_packet_init(struct tftp_ack_packet packet[static 1], uint16_t block_number);

ssize_t tftp_rrq_packet_init(struct tftp_rrq_packet packet[static 1],
                             size_t n,
                             const char filename[static n],
                             enum tftp_mode mode,
                             struct tftp_option options[static TFTP_OPTION_TOTAL_OPTIONS]);
