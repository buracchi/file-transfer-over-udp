#pragma once
/* FTCP */

enum ftcp_type { INVALID_TYPE, COMMAND, RESPONSE };
enum ftcp_operation { INVALID_OPERATION, LIST, GET, PUT };

extern enum ftcp_type ftcp_get_type(char* message);

extern enum ftcp_operation ftcp_get_operation(char* message);

extern char* ftcp_get_arg(char* message);
