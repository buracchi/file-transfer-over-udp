#pragma once

#include <stdint.h>

/*
* File Transfer Communication Protocol
* 
* +------------------------------------------------------------------+
* |                            FTCP Packet                           |
* +------+------------------+-----------+-------------+--------------+
* |  1 B |        1 B       |   256 B   |     8 B     |   Variable   |
* +------+------------------+-----------+-------------+--------------+
* | Type | Operation/Result | File name | File length | File content |
* +------+------------------+-----------+-------------+--------------+
* 
* Type: Message type
* Operation: Message operation
* File name: NULL terminated string (up to 255 characters)
* File length: File length (up to 16 EiB)
* 
*/

typedef uint8_t* ftcp_message_t;

enum ftcp_type { INVALID_TYPE, COMMAND, RESPONSE };
enum ftcp_operation { INVALID_OPERATION, LIST, GET, PUT };
enum ftcp_result { INVALID_RESULT, SUCCESS, ERROR };

extern ftcp_message_t ftcp_message_init(
	enum ftcp_type type,
	enum ftcp_operation operation,
	const char* filename, 
	uint64_t file_content_lenght,
	const uint8_t* file_content
);

extern enum ftcp_type ftcp_get_type(ftcp_message_t ftcp_message);

extern enum ftcp_operation ftcp_get_operation(ftcp_message_t ftcp_message);

extern char* ftcp_get_filename(ftcp_message_t ftcp_message);

extern uint64_t ftcp_get_file_length(ftcp_message_t ftcp_message);

extern uint8_t* ftcp_get_file_content(ftcp_message_t ftcp_message);
