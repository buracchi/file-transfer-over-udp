#pragma once

#include <stdint.h>

/*******************************************************************************
*                     File Transfer Communication Protocol                     *
*******************************************************************************/
/*
* Communication between hosts in the application occurs through the exchange 
* of FTCP packets.
* 
* The FTCP packets are subdivided in:
* 
*	- Preamble Packets: precede each Data Packet and provide information to the
* 						host about how to process data packets.
*
*		+----------------------------------------------------+
*		|                FTCP Preamble Packet                |
*		+------+------------------+-----+--------------------+
*		|  1 B |        1 B       |256 B|         8 B        |
*		+------+------------------+-----+--------------------+
*		| Type | Operation/Result | Arg | Data Packet Length |
*		+------+------------------+-----+--------------------+
* 
*						Type:				Message type
*						Operation/Result:	Message operation
*						Arg:				Additional argument
*						Data length:		Data Packet length (up to 16 EiB)
* 
*	- Data Packets:		contain an array of bytes that can represent different 
*						types of data depending on the processing context,
*						each Data Packet MUST be preceded by a Preamble Packet.
* 
*		+----------------------------------------------+
*		|               FTCP Data Packet               |
*		+----------------------------------------------+
*		|             Data Packet Length B             |
*		+----------------------------------------------+
*		|                     Data                     |
*		+----------------------------------------------+
* 
*						Data:				Data
* 
*/

typedef uint8_t *ftcp_pp_t;
typedef uint8_t *ftcp_dp_t;

enum ftcp_type {
    INVALID_TYPE, COMMAND, RESPONSE
};
enum ftcp_operation {
    INVALID_OPERATION, LIST, GET, PUT
};
enum ftcp_result {
    INVALID_RESULT, SUCCESS, ERROR, FILE_EXIST, FILE_NOT_EXIST, INVALID_ARGUMENT
};

extern ftcp_pp_t ftcp_pp_init(
        enum ftcp_type type,
        enum ftcp_operation operation,
        uint8_t arg[256],
        uint64_t dplen
);

#define ftcp_pp_size() 266

extern enum ftcp_type ftcp_get_type(ftcp_pp_t ftcp_packet);

extern enum ftcp_operation ftcp_get_operation(ftcp_pp_t ftcp_packet);

extern enum ftcp_result ftcp_get_result(ftcp_pp_t ftcp_packet);

extern uint8_t *ftcp_get_arg(ftcp_pp_t ftcp_packet);

extern uint64_t ftcp_get_dplen(const const const ftcp_pp_t ftcp_packet);
