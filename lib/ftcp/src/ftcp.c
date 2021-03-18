#include "ftcp.h"

#include <stdlib.h>
#include <string.h>

static uint64_t _max(uint64_t a, uint64_t b) {
	return a > b ? a : b;
}

extern ftcp_message_t ftcp_message_init(enum ftcp_type type, enum ftcp_operation operation, const char* filename, uint64_t file_content_lenght, const uint8_t* file_content) {
	ftcp_message_t message;
	message = malloc(sizeof * message * (266 + _max(1, file_content_lenght)));
	if (message) {
		memset(message, 0, 266 + _max(1, file_content_lenght));
		message[0] = type;
		message[1] = operation;
		if (filename) {
			strncpy(message + 2, filename, 255);
		}
		for (int i = 0; i < 8; i++) {
			message[258 + i] = (file_content_lenght >> 8 * (7 - i)) & 0xFF;
		}
		if (file_content) {
			memcpy(message + 266, file_content, file_content_lenght);
		}
	}
	return message;
}

extern inline uint64_t ftcp_message_length(ftcp_message_t ftcp_message) {
	return 266 + _max(1, ftcp_get_file_length(ftcp_message));
}

extern inline enum ftcp_type ftcp_get_type(ftcp_message_t ftcp_message) {
	return ftcp_message[0];
}

extern inline enum ftcp_operation ftcp_get_operation(ftcp_message_t ftcp_message) {
	return ftcp_message[1];
}

extern inline char* ftcp_get_filename(ftcp_message_t ftcp_message) {
	return ftcp_message + 2;
}

extern uint64_t ftcp_get_file_length(ftcp_message_t ftcp_message) {
	uint64_t result = 0;
	for (int i = 0; i < 8; i++) {
		result += (uint64_t)ftcp_message[258 + i] << 8 * (7 - i);
	}
	return result;
}

extern inline uint8_t* ftcp_get_file_content(ftcp_message_t ftcp_message) {
	return ftcp_message + 266;
}
