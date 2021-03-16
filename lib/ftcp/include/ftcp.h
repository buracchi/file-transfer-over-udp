#pragma once
/* FTCP */

enum type { INVALID_TYPE, COMMAND, RESPONSE };
enum operation { INVALID_OPERATION, LIST, GET, PUT };

extern enum type get_type(char* message);

extern enum operation get_operation(char* message);

extern char* get_arg(char* message);
