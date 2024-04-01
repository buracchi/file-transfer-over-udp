#include "tftp.h"

extern char const *tftp_error_to_string(enum tftp_error error) {
	switch (error) {
	case TFTP_ERROR_UNDEFINED:
		return "Not defined, see error message (if any).";
	case TFTP_ERROR_FILE_NOT_FOUND:
		return "File not found.";
	case TFTP_ERROR_ACCESS_VIOLATION:
		return "Access violation.";
	case TFTP_ERROR_NOMEM:
		return "Disk full or allocation exceeded.";
	case TFTP_ERROR_ILLEGAL_OPERATION:
		return "Illegal TFTP operation.";
	case TFTP_ERROR_UNKNOWN_TRANSFER_ID:
		return "Unknown transfer ID.";
	case TFTP_ERROR_FILE_ALREADY_EXISTS:
		return "File already exists.";
	case TFTP_ERROR_NO_SUCH_USER:
		return "No such user.";
	default:
		break;
	}
	return "Invalid error code";
}
