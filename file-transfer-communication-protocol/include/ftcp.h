#ifndef FTCP_H_INCLUDED
#define FTCP_H_INCLUDED

#include <stdint.h>
#include <stddef.h>
#include <string.h>

/*******************************************************************************
*                     File Transfer Communication Protocol                     *
*******************************************************************************/
/**
 * Communication between hosts in the application occurs through the exchange
 * of FTCP packets.
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
 *	Type:			Message type
 *	Operation/Result:	Message operation/Request result
 *	Arg:			Additional argument
 *	Data length:		Data Packet length (up to 16 EiB)
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
 *	Data:			Data
 *
 */

#define FTCP_PREAMBLE_PACKET_SIZE			266
#define FTCP_PREAMBLE_PACKET_TYPE_SIZE			1
#define FTCP_PREAMBLE_PACKET_OPERATION_SIZE		1
#define FTCP_PREAMBLE_PACKET_RESULT_SIZE		1
#define FTCP_PREAMBLE_PACKET_ARG_SIZE			256
#define FTCP_PREAMBLE_PACKET_DATA_PACKET_LENGHT_SIZE	8

typedef uint8_t ftcp_preamble_packet_t[FTCP_PREAMBLE_PACKET_SIZE];
typedef uint8_t* ftcp_data_packet_t;

struct ftcp_type {
	uint8_t value;
};

struct ftcp_operation {
	uint8_t value;
};

struct ftcp_result {
	uint8_t value;
};

#define FTCP_TYPE_INVALID_VALUE		0
#define FTCP_TYPE_COMMAND_VALUE		1
#define FTCP_TYPE_RESPONSE_VALUE	2

#define FTCP_TYPE_INVALID	(struct ftcp_type) { FTCP_TYPE_INVALID_VALUE }
#define FTCP_TYPE_COMMAND	(struct ftcp_type) { FTCP_TYPE_COMMAND_VALUE }
#define FTCP_TYPE_RESPONSE	(struct ftcp_type) { FTCP_TYPE_RESPONSE_VALUE }

#define FTCP_OPERATION_INVALID_VALUE	0
#define FTCP_OPERATION_LIST_VALUE	1
#define FTCP_OPERATION_GET_VALUE	2
#define FTCP_OPERATION_PUT_VALUE	3

#define FTCP_OPERATION_INVALID	(struct ftcp_operation) { FTCP_OPERATION_INVALID_VALUE }
#define FTCP_OPERATION_LIST	(struct ftcp_operation) { FTCP_OPERATION_LIST_VALUE }
#define FTCP_OPERATION_GET	(struct ftcp_operation) { FTCP_OPERATION_GET_VALUE }
#define FTCP_OPERATION_PUT	(struct ftcp_operation) { FTCP_OPERATION_PUT_VALUE }

#define FTCP_RESULT_INVALID_VALUE		0
#define FTCP_RESULT_SUCCESS_VALUE		1
#define FTCP_RESULT_ERROR_VALUE			2
#define FTCP_RESULT_FILE_EXIST_VALUE		3
#define FTCP_RESULT_FILE_NOT_EXIST_VALUE	4
#define FTCP_RESULT_INVALID_ARGUMENT_VALUE	5

#define FTCP_RESULT_INVALID		(struct ftcp_result) { FTCP_RESULT_INVALID_VALUE }
#define FTCP_RESULT_SUCCESS		(struct ftcp_result) { FTCP_RESULT_SUCCESS_VALUE }
#define FTCP_RESULT_ERROR		(struct ftcp_result) { FTCP_RESULT_ERROR_VALUE }
#define FTCP_RESULT_FILE_EXIST		(struct ftcp_result) { FTCP_RESULT_FILE_EXIST_VALUE }
#define FTCP_RESULT_FILE_NOT_EXIST	(struct ftcp_result) { FTCP_RESULT_FILE_NOT_EXIST_VALUE }
#define FTCP_RESULT_INVALID_ARGUMENT	(struct ftcp_result) { FTCP_RESULT_INVALID_ARGUMENT_VALUE }

static inline void ftcp_preamble_packet_init_internal(ftcp_preamble_packet_t* packet,
						      struct ftcp_type type,
						      uint8_t operation_or_result,
						      const void* arg,
						      uint64_t data_packet_length) {
	memcpy(&((*packet)[0]), &type, FTCP_PREAMBLE_PACKET_TYPE_SIZE);
	memcpy(&((*packet)[1]), &operation_or_result, 1);
	if (arg) {
		memcpy(&((*packet)[2]), arg, FTCP_PREAMBLE_PACKET_ARG_SIZE);
	}
	else {
		memset(&((*packet)[2]), 0, FTCP_PREAMBLE_PACKET_ARG_SIZE);
	}
	for (int i = 0; i < 8; i++) {
		memset(&((*packet)[258 + i]), (data_packet_length >> (8 * (7 - i))) & 0xFF, 1);
	}
}

static inline void ftcp_preamble_packet_operation_init(ftcp_preamble_packet_t* packet,
						       struct ftcp_type type,
						       struct ftcp_operation operation,
						       const void* arg,
						       uint64_t data_packet_length) {
	ftcp_preamble_packet_init_internal(packet, type, operation.value, arg, data_packet_length);
}

static inline void ftcp_preamble_packet_result_init(ftcp_preamble_packet_t* packet,
						    struct ftcp_type type,
						    struct ftcp_result result,
						    const void* arg,
						    uint64_t data_packet_length) {
	ftcp_preamble_packet_init_internal(packet, type, result.value, arg, data_packet_length);
}

#define ftcp_preamble_packet_init(packet, type, operation_or_result, arg, data_packet_length)	\
_Generic(operation_or_result,									\
	struct ftcp_operation : ftcp_preamble_packet_operation_init,				\
	   struct ftcp_result : ftcp_preamble_packet_result_init				\
)((packet), (type), (operation_or_result), (arg), (data_packet_length))

static inline uint8_t ftcp_preamble_packet_type(ftcp_preamble_packet_t const packet) {
	return packet[0];
}

static inline uint8_t ftcp_preamble_packet_operation(ftcp_preamble_packet_t const packet) {
	return packet[1];
}

static inline uint8_t ftcp_preamble_packet_result(ftcp_preamble_packet_t const packet) {
	return packet[1];
}

static inline uint8_t* ftcp_preamble_packet_arg(ftcp_preamble_packet_t packet) {
	return &(packet[2]);
}

static inline uint64_t ftcp_preamble_packet_data_packet_length(ftcp_preamble_packet_t const packet) {
	uint64_t result = 0;
	for (int i = 0; i < 8; i++) {
		result += ((uint64_t) packet[258 + i] << (8 * (7 - i))) & 0xFF;
	}
	return result;
}

#endif // FTCP_H_INCLUDED
