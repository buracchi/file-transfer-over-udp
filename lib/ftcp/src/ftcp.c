#include "ftcp.h"

extern enum ftcp_type ftcp_get_type(char* message) {
	return message[0];
}

extern enum ftcp_operation ftcp_get_operation(char* message) {
	return message[0] == INVALID_TYPE ? INVALID_OPERATION : message[1];
}

extern char* ftcp_get_arg(char* message) {
	return message + 2;
}
