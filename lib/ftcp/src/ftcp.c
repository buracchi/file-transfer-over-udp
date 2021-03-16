#include "ftcp.h"

extern enum type get_type(char* message) {
	return message[0];
}

extern enum operation get_operation(char* message) {
	return message[0] == INVALID_TYPE ? INVALID_OPERATION : message[1];
}

extern char* get_arg(char* message) {
	return message + 2;
}
