#include "utilities.h"

#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <Windows.h>
#include <tchar.h>
#endif

#ifdef _WIN32
int vasprintf(LPTSTR* str, LPCTSTR format, va_list args){
	size_t size;
	LPTSTR buff;
	va_list tmp;
	va_copy(tmp, args);
	size = _vsctprintf(format, tmp);
	va_end(tmp);
	if (size == -1){
		return -1;
	}
	if ((buff = (LPTSTR)malloc(sizeof(TCHAR) * (size + 1))) == NULL){
		return -1;
	}
	size = _vstprintf_s(buff, size + 1, format, args);
	*str = buff;
	return (int)size;
}
#elif __unix__
int vasprintf(char** str, const char* format, va_list args) {
	int size;
	char* buff;
	va_list tmp;
	va_copy(tmp, args);
	size = vsnprintf(NULL, 0, format, tmp);
	va_end(tmp);
	if (size == -1) {
		return -1;
	}
	if ((buff = (char*)malloc(sizeof(char) * (size_t)(size + 1))) == NULL) {
		return -1;
	}
	size = vsprintf(buff, format, args);
	*str = buff;
	return size;
}
#endif

#ifdef _WIN32
int asprintf(LPTSTR* str, LPCTSTR format, ...){
	size_t size;
	va_list args;
	va_start(args, format);
	size = vasprintf(str, format, args);
	va_end(args);
	return (int)size;
}
#elif __unix__
int asprintf(char** str, const char* format, ...) {
	int size;
	va_list args;
	va_start(args, format);
	size = vasprintf(str, format, args);
	va_end(args);
	return (int)size;
}
#endif

int strtoi(char* str, int* result) {
	char* endptr;
	long val;

	errno = 0;    // To distinguish success/failure after call
	val = strtol(str, &endptr, 10);

	// Check for various possible errors

	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN))
		|| (errno != 0 && val == 0)) {
		return 1;
	}

	if (endptr == str) {
		errno = EINVAL;
		return 1;
	}
	// If we got here, strtol() successfully parsed a number

	*result = (int)val;

	return 0;
}

int str_to_uint16(const char* str, uint16_t* result) {
	char* endptr;
	long val;
	errno = 0;    // To distinguish success/failure after call
	val = strtol(str, &endptr, 10);
	if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {
		goto fail;
	}
	if (endptr == str) {
		errno = EINVAL;
		goto fail;
	}
	if (val > UINT16_MAX || val < 0) {
		errno = ERANGE;
		goto fail;
	}
	*result = (uint16_t)val;
	return 0;
fail:
	return 1;
}

bool is_directory(char* pathname) {
	struct stat dir_stat;
	return (!stat(pathname, &dir_stat) && S_ISDIR(dir_stat.st_mode));
}
