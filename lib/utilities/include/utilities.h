#pragma once

#include <stdarg.h>
#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef _WIN32
int asprintf(LPTSTR* str, LPCTSTR format, ...);
#elif __unix__
int asprintf(char** str, const char* format, ...);
#endif

#define forever for(;;)

#define max(a, b) a > b ? a : b

#define streq(expected, actual) !strncasecmp(actual, expected, max(strlen(expected), strlen(actual)))
#define __tostr(statement) #statement
#define tostr(statement) __tostr(statement)

int strtoi(char* str, int* result);

bool is_directory(char* pathname);
