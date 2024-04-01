#ifndef FTCP_H_INCLUDED
#define FTCP_H_INCLUDED

#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifndef typeof
#define typeof __typeof
#endif

/*******************************************************************************
 *                     File Transfer Communication Protocol                    *
 *******************************************************************************
 *
 * The File Transfer Communication Protocol is an application layer level
 * communication protocol which allows files transfer on a client-server model
 * architecture network.
 *
 * Communication between hosts can occurs through the exchange of FTCP packets.
 *
 * The FTCP packets are subdivided in:
 *
 *	- Preamble Packets:
 *
 *	Precede each Data Packet and provide information to the host about how
 *	to process data packets.
 *
 *	+----------------------------------------------------+
 *	|                FTCP Preamble Packet                |
 *	+------+------------------+-----+--------------------+
 *	|  1 B |        1 B       |256 B|         8 B        |
 *	+------+------------------+-----+--------------------+
 *	| Type | Operation/Result | Arg | Data Packet Length |
 *	+------+------------------+-----+--------------------+
 *
 *	Type:			Represents the nature of the message exchange,
 *				it SHOULD be set to a value representing the
 *				following states: COMMAND, RESPONSE.
 *
 *	Operation/Result:	Represents the required operation for COMMAND
 *				type packets or the result of an operation for
 *				RESPONSE type packets.
 *				In a COMMAND type packet it SHOULD be set to a
 *				value representing the following states:
 *				LIST, GET, PUT.
 *				In a RESPONSE type packet it SHOULD be set to a
 *				value representing the following states:
 *				SUCCESS, ERROR, FILE_EXISTS, FILE_NOT_EXISTS,
 *				INVALID_ARGUMENT.
 *
 *	Arg:			Additional data
 *
 *	Data Packet length:	Encoded size of the following Data Packet
 *				(up to 16 EiB)
 *
 *	- Data Packets:
 *
 *	Contain an array of bytes that can represent different types of data
 *	depending on the processing context, each Data Packet MUST be preceded
 *	by a Preamble Packet.
 *
 *	+----------------------------------------------+
 *	|               FTCP Data Packet               |
 *	+----------------------------------------------+
 *	|             Data Packet Length B             |
 *	+----------------------------------------------+
 *	|                     Data                     |
 *	+----------------------------------------------+
 *
 * The File Transfer Communication Protocol allows the following operations:
 *
 *	- LIST:
 *
 *	A client SHALL be able to require the list of available files from a
 *	server by sending a List packet.
 *	A List packet is a preamble packet containing: the COMMAND value in
 *	the Type field and the LIST value in the Operation field.
 *
 *	A server receiving a List packet is REQUIRED to reply sending a
 *	preamble packet containing: the RESPONSE value in the Type field,
 *	the SUCCESS value in the Result field and the size of the next packet
 *	in the Data Packet length field; followed by a data packet containing
 *	a representation of the available file identifiers back to the client.
 *
 *	- GET:
 *
 *	A client SHALL be able to require an available file from a server
 *	by sending a Get packet.
 *	A Get packet is a preamble packet containing: the COMMAND value in
 *	the Type field, the GET value in the Operation field and a
 *	representation of the required file identifier in the Arg field.
 *
 *	A server receiving a Get packet is REQUIRED to reply sending a preamble
 *	packet which SHOULD contain: the RESPONSE value in the Type field, the
 *	FILE_EXISTS value in the Result field if the file required is
 *	available or FILE_NOT_EXISTS otherwise and the size of the next packet
 *	in the Data Packet length field; followed by a data packet containing
 *	the content of the required file back to the client.
 *
 *	- PUT:
 *
 *	A client SHALL be able to require the insertion of a file (which will
 *	then be made available) into a server by sending a Put packet.
 *	A Put packet is a preamble packet containing: the COMMAND value in
 *	the Type field, the PUT value in the Operation field, a	representation
 *	of the required file identifier in the Arg field and the size of the
 *	next packet in the Data Packet length field; followed by a data packet
 *	containing the content of the file.
 *
 *	A server receiving a Put packet is REQUIRED to reply sending a preamble
 *	packet which SHOULD contain: the RESPONSE value in the Type field and
 *	the FILE_EXISTS value in the Result field if the file is available or
 *	FILE_NOT_EXISTS otherwise. After receiving the file contents the server
 *	MUST make it available and send back to the client a preamble packet
 *	containing: the RESPONSE value in the Type field and the SUCCESS value
 *	in the Result field.
 *
 */

/*******************************************************************************
 *                                  FTCP Type                                  *
 ******************************************************************************/

struct ftcp_type {
	uint8_t value;
};

typedef typeof(((struct ftcp_type *)0)->value) ftcp_type_value_t;

#define FTCP_TYPE_INVALID_VALUE  0
#define FTCP_TYPE_COMMAND_VALUE  1
#define FTCP_TYPE_RESPONSE_VALUE 2

#define FTCP_TYPE_INVALID                                                      \
	(struct ftcp_type) { FTCP_TYPE_INVALID_VALUE }
#define FTCP_TYPE_COMMAND                                                      \
	(struct ftcp_type) { FTCP_TYPE_COMMAND_VALUE }
#define FTCP_TYPE_RESPONSE                                                     \
	(struct ftcp_type) { FTCP_TYPE_RESPONSE_VALUE }

/*******************************************************************************
 *                               FTCP Operation                                *
 ******************************************************************************/

struct ftcp_operation {
	uint8_t value;
};

typedef typeof(((struct ftcp_operation *)0)->value) ftcp_operation_value_t;

#define FTCP_OPERATION_INVALID_VALUE 0
#define FTCP_OPERATION_LIST_VALUE    1
#define FTCP_OPERATION_GET_VALUE     2
#define FTCP_OPERATION_PUT_VALUE     3

#define FTCP_OPERATION_INVALID                                                 \
	(struct ftcp_operation) { FTCP_OPERATION_INVALID_VALUE }
#define FTCP_OPERATION_LIST                                                    \
	(struct ftcp_operation) { FTCP_OPERATION_LIST_VALUE }
#define FTCP_OPERATION_GET                                                     \
	(struct ftcp_operation) { FTCP_OPERATION_GET_VALUE }
#define FTCP_OPERATION_PUT                                                     \
	(struct ftcp_operation) { FTCP_OPERATION_PUT_VALUE }

/*******************************************************************************
 *                                 FTCP Result                                 *
 ******************************************************************************/

struct ftcp_result {
	uint8_t value;
};

typedef typeof(((struct ftcp_result *)0)->value) ftcp_result_value_t;

#define FTCP_RESULT_SUCCESS_VALUE          0
#define FTCP_RESULT_ERROR_VALUE            1
#define FTCP_RESULT_FILE_EXISTS_VALUE      2
#define FTCP_RESULT_FILE_NOT_EXISTS_VALUE  3
#define FTCP_RESULT_INVALID_ARGUMENT_VALUE 4

#define FTCP_RESULT_SUCCESS                                                    \
	(struct ftcp_result) { FTCP_RESULT_SUCCESS_VALUE }
#define FTCP_RESULT_ERROR                                                      \
	(struct ftcp_result) { FTCP_RESULT_ERROR_VALUE }
#define FTCP_RESULT_FILE_EXISTS                                                \
	(struct ftcp_result) { FTCP_RESULT_FILE_EXISTS_VALUE }
#define FTCP_RESULT_FILE_NOT_EXISTS                                            \
	(struct ftcp_result) { FTCP_RESULT_FILE_NOT_EXISTS_VALUE }
#define FTCP_RESULT_INVALID_ARGUMENT                                           \
	(struct ftcp_result) { FTCP_RESULT_INVALID_ARGUMENT_VALUE }

/*******************************************************************************
 *                                   FTCP Arg                                  *
 ******************************************************************************/

struct ftcp_arg {
	uint8_t value[256];
};

typedef struct ftcp_arg *ftcp_arg_t;

/*******************************************************************************
 *                             FTCP Preamble Packet                            *
 ******************************************************************************/

#define FTCP_SIZE_MAX_BETWEEN(S, T)                                            \
	((S) + (((T) - (S)) & (((((S) - (T)) >> (sizeof(size_t) * CHAR_BIT - 1)) & UCHAR_MAX) * SIZE_MAX)))

#define FTCP_PREAMBLE_PACKET_TYPE_SIZE      sizeof(struct ftcp_type)
#define FTCP_PREAMBLE_PACKET_OPERATION_SIZE sizeof(struct ftcp_operation)
#define FTCP_PREAMBLE_PACKET_RESULT_SIZE    sizeof(struct ftcp_result)
#define FTCP_PREAMBLE_PACKET_SECOND_FIELD_SIZE                                 \
	FTCP_SIZE_MAX_BETWEEN(FTCP_PREAMBLE_PACKET_OPERATION_SIZE, FTCP_PREAMBLE_PACKET_RESULT_SIZE)
#define FTCP_PREAMBLE_PACKET_ARG_SIZE                sizeof(struct ftcp_arg)
#define FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_SIZE 8
#define FTCP_PREAMBLE_PACKET_SIZE                                              \
	(size_t)(FTCP_PREAMBLE_PACKET_TYPE_SIZE + FTCP_PREAMBLE_PACKET_SECOND_FIELD_SIZE + FTCP_PREAMBLE_PACKET_ARG_SIZE + FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_SIZE)

#define FTCP_PREAMBLE_PACKET_TYPE_OFFSET 0
#define FTCP_PREAMBLE_PACKET_SECOND_FIELD_OFFSET                               \
	((FTCP_PREAMBLE_PACKET_TYPE_OFFSET) + (FTCP_PREAMBLE_PACKET_TYPE_SIZE))
#define FTCP_PREAMBLE_PACKET_OPERATION_OFFSET                                  \
	FTCP_PREAMBLE_PACKET_SECOND_FIELD_OFFSET
#define FTCP_PREAMBLE_PACKET_RESULT_OFFSET                                     \
	FTCP_PREAMBLE_PACKET_SECOND_FIELD_OFFSET
#define FTCP_PREAMBLE_PACKET_ARG_OFFSET                                        \
	((FTCP_PREAMBLE_PACKET_SECOND_FIELD_OFFSET) + (FTCP_PREAMBLE_PACKET_SECOND_FIELD_SIZE))
#define FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_OFFSET                         \
	((FTCP_PREAMBLE_PACKET_ARG_OFFSET) + (FTCP_PREAMBLE_PACKET_ARG_SIZE))

typedef uint8_t ftcp_preamble_packet_t[FTCP_PREAMBLE_PACKET_SIZE];

/******************************************************************************
 *                               FTCP Utilities                               *
 *****************************************************************************/

static inline uint8_t *ftcp_encode_data_packet_length(uint8_t (*dest)[FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_SIZE], uint64_t data_packet_length) {
	const size_t dest_size = FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_SIZE;
	for (size_t i = 0; i < dest_size; i++) {
		uint8_t encoded_byte;
		encoded_byte = (data_packet_length >> (dest_size * (dest_size - 1 - i))) & UINT8_MAX;
		memset(&((*dest)[i]), encoded_byte, 1);
	}
	return (uint8_t *)dest;
}

static inline uint64_t ftcp_decode_data_packet_length(uint8_t const src[const FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_SIZE]) {
	const size_t src_size = FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_SIZE;
	uint64_t result = 0;
	for (size_t i = 0; i < src_size; i++) {
		result += (((uint64_t)src[i] & UINT8_MAX) << (src_size * (src_size - 1 - i)));
	}
	return result;
}

/*******************************************************************************
 *                                FTCP Functions                               *
 ******************************************************************************/

static inline void ftcp_preamble_packet_init_internal(ftcp_preamble_packet_t *packet, struct ftcp_type type, uint8_t second_field_value, size_t second_field_value_size, struct ftcp_arg const *arg, uint64_t data_packet_length) {
	struct ftcp_arg zero_arg = { 0 };
	uint8_t encoded_data_packet_length[FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_SIZE] = { 0 };
	ftcp_encode_data_packet_length(&encoded_data_packet_length, data_packet_length);
	memcpy(&((*packet)[FTCP_PREAMBLE_PACKET_TYPE_OFFSET]), &type, FTCP_PREAMBLE_PACKET_TYPE_SIZE);
	memcpy(&((*packet)[FTCP_PREAMBLE_PACKET_SECOND_FIELD_OFFSET]), &second_field_value, second_field_value_size);
	memcpy(&((*packet)[FTCP_PREAMBLE_PACKET_ARG_OFFSET]), arg ? arg : &zero_arg, FTCP_PREAMBLE_PACKET_ARG_SIZE);
	memcpy(&((*packet)[FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_OFFSET]), &encoded_data_packet_length, FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_SIZE);
}

static inline void ftcp_preamble_packet_operation_init(ftcp_preamble_packet_t *packet, struct ftcp_type type, struct ftcp_operation operation, struct ftcp_arg const *arg, uint64_t data_packet_length) {
	ftcp_preamble_packet_init_internal(packet, type, operation.value, sizeof(operation), arg, data_packet_length);
}

static inline void ftcp_preamble_packet_result_init(ftcp_preamble_packet_t *packet, struct ftcp_type type, struct ftcp_result result, struct ftcp_arg const *arg, uint64_t data_packet_length) {
	ftcp_preamble_packet_init_internal(packet, type, result.value, sizeof(result), arg, data_packet_length);
}

#define ftcp_preamble_packet_init(packet, type, operation_or_result, arg, data_packet_length) \
	_Generic((operation_or_result), struct ftcp_operation                                 \
	         : ftcp_preamble_packet_operation_init, struct ftcp_result                    \
	         : ftcp_preamble_packet_result_init)((packet), (type), (operation_or_result), (arg), (data_packet_length))

static inline ftcp_type_value_t ftcp_preamble_packet_type(ftcp_preamble_packet_t const packet) {
	return packet[FTCP_PREAMBLE_PACKET_TYPE_OFFSET];
}

static inline ftcp_operation_value_t ftcp_preamble_packet_operation(ftcp_preamble_packet_t const packet) {
	return packet[FTCP_PREAMBLE_PACKET_OPERATION_OFFSET];
}

static inline ftcp_result_value_t ftcp_preamble_packet_result(ftcp_preamble_packet_t const packet) {
	return packet[FTCP_PREAMBLE_PACKET_RESULT_OFFSET];
}

static inline ftcp_arg_t ftcp_preamble_packet_arg(ftcp_preamble_packet_t packet) {
	return (ftcp_arg_t) & (packet[FTCP_PREAMBLE_PACKET_ARG_OFFSET]);
}

static inline uint64_t ftcp_preamble_packet_data_packet_length(ftcp_preamble_packet_t const packet) {
	return ftcp_decode_data_packet_length(&packet[FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGTH_OFFSET]);
}

#endif // FTCP_H_INCLUDED
