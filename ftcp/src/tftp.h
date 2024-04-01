#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define TFTP_PACKET_TYPE_LRQ                                                   \
	(uint8_t[2]) { 0x0, 0x0 }
#define TFTP_PACKET_TYPE_RRQ                                                   \
	(uint8_t[2]) { 0x0, 0x1 }
#define TFTP_PACKET_TYPE_WRQ                                                   \
	(uint8_t[2]) { 0x0, 0x2 }
#define TFTP_PACKET_TYPE_DATA                                                  \
	(uint8_t[2]) { 0x0, 0x3 }
#define TFTP_PACKET_TYPE_ACK                                                   \
	(uint8_t[2]) { 0x0, 0x4 }
#define TFTP_PACKET_TYPE_ERROR                                                 \
	(uint8_t[2]) { 0x0, 0x5 }

typedef uint8_t *tftp_packet_t;

enum tftp_error {
	TFTP_ERROR_UNDEFINED = 0,
	TFTP_ERROR_FILE_NOT_FOUND = 1,
	TFTP_ERROR_ACCESS_VIOLATION = 2,
	TFTP_ERROR_NOMEM = 3,
	TFTP_ERROR_ILLEGAL_OPERATION = 4,
	TFTP_ERROR_UNKNOWN_TRANSFER_ID = 5,
	TFTP_ERROR_FILE_ALREADY_EXISTS = 6,
	TFTP_ERROR_NO_SUCH_USER = 7
};

enum tftp_transfer_mode {
	TFTP_TRANSFER_MODE_NETASCII,
	TFTP_TRANSFER_MODE_OCTET
};

static uint8_t tftp_transfer_mode_netascii[] = { 'n', 'e', 't', 'a', 's',
	                                         'c', 'i', 'i', '\0' };

static uint8_t tftp_transfer_mode_octet[] = { 'o', 'c', 't', 'e', 't', '\0' };

extern char const *tftp_error_to_string(enum tftp_error);

static inline uint8_t *
get_transfer_mode_value(enum tftp_transfer_mode transfer_mode) {
	switch (transfer_mode) {
	case TFTP_TRANSFER_MODE_NETASCII:
		return tftp_transfer_mode_netascii;
	case TFTP_TRANSFER_MODE_OCTET:
		return tftp_transfer_mode_octet;
	default:
		break;
	}
	return (uint8_t *)"";
}

static inline tftp_packet_t
tftp_get_packet_rrq(char const *filename,
                    enum tftp_transfer_mode transfer_mode) {
	uint8_t *transfer_mode_value = get_transfer_mode_value(transfer_mode);
	size_t filename_len = strlen(filename);
	size_t transfer_mode_len = strlen(transfer_mode_value);
}

static inline void tftp_packet_destroy(tftp_packet_t packet) {
	free(packet);
}
