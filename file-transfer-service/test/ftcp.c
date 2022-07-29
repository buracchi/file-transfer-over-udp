#include "ftcp.h"

#include <stdbool.h>
#include <stdlib.h>

extern bool is_preamble_packet_type_correctly_encapsulated() {
	ftcp_preamble_packet_t packet;
	ftcp_type_value_t actual;
	ftcp_type_value_t expected = FTCP_TYPE_COMMAND_VALUE;
	ftcp_preamble_packet_init(&packet, FTCP_TYPE_COMMAND, FTCP_OPERATION_GET, NULL, 0);
	actual = ftcp_preamble_packet_type(packet);
	return actual == expected;
}

extern bool is_preamble_packet_operation_correctly_encapsulated() {
	ftcp_preamble_packet_t packet;
	ftcp_operation_value_t actual;
	ftcp_operation_value_t expected = FTCP_OPERATION_GET_VALUE;
	ftcp_preamble_packet_init(&packet, FTCP_TYPE_COMMAND, FTCP_OPERATION_GET, NULL, 0);
	actual = ftcp_preamble_packet_operation(packet);
	return actual == expected;
}

extern bool is_preamble_packet_result_correctly_encapsulated() {
	ftcp_preamble_packet_t packet;
	ftcp_operation_value_t actual;
	ftcp_operation_value_t expected = FTCP_RESULT_FILE_EXISTS_VALUE;
	ftcp_preamble_packet_init(&packet, FTCP_TYPE_RESPONSE, FTCP_RESULT_FILE_EXISTS, NULL, 0);
	actual = ftcp_preamble_packet_result(packet);
	return actual == expected;
}

extern bool is_preamble_packet_arg_correctly_encapsulated() {
	uint8_t buffer[FTCP_PREAMBLE_PACKET_ARG_SIZE];
	ftcp_preamble_packet_t packet;
	ftcp_arg_t actual;
	ftcp_arg_t expected = (ftcp_arg_t) &buffer;
	srand(0);
	for (size_t i = 0; i < FTCP_PREAMBLE_PACKET_ARG_SIZE; i++) {
		buffer[i] = rand() % 0x100;
	}
	ftcp_preamble_packet_init(&packet, FTCP_TYPE_RESPONSE, FTCP_RESULT_FILE_EXISTS, expected, 0);
	actual = ftcp_preamble_packet_arg(packet);
	return memcmp(actual, expected, sizeof(struct ftcp_arg)) == 0;
}

extern bool is_preamble_packet_data_packet_lenght_correctly_encapsulated() {
	ftcp_preamble_packet_t packet;
	uint64_t actual;
	uint64_t expected = 123456789;
	ftcp_preamble_packet_init(&packet, FTCP_TYPE_RESPONSE, FTCP_RESULT_SUCCESS, NULL, 123456789);
	actual = ftcp_preamble_packet_data_packet_length(packet);
	return actual == expected;
}
