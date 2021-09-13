#pragma once

#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef _WIN32
int asprintf(LPTSTR* str, LPCTSTR format, ...);
#elif __unix__

int asprintf(char **str, const char *format, ...);

#endif

#define max(a, b) ({    \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b; })

#define __tostr(statement) #statement
#define tostr(statement) __tostr(statement)

typedef struct cmn_pair {
    void *first;
    void *second;
} *cmn_pair_t;

int strtoi(char *str, int *result);

int str_to_uint16(const char *str, uint16_t *result);

static inline bool streq(const char *str1, const char *str2) {
    return (strlen(str1) == strlen(str2)) && (!strcmp(str1, str2));
}

bool is_directory(char *pathname);

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
        const typeof(((type *)0)->member) * __mptr = (ptr);     \
        (type *)((char *)__mptr - offsetof(type, member)); })
