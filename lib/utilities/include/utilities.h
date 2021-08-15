#pragma once

#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#include <Windows.h>
#endif

#ifdef _WIN32
int asprintf(LPTSTR* str, LPCTSTR format, ...);
#elif __unix__
int asprintf(char** str, const char* format, ...);
#endif

#define max(a, b) ({    \
    typeof(a) _a = (a); \
    typeof(b) _b = (b); \
    _a > _b ? _a : _b })

#define __tostr(statement) #statement
#define tostr(statement) __tostr(statement)
#define streq(expected, actual) !strncasecmp(actual, expected, max(strlen(expected), strlen(actual)))

struct cmn_pair {
	void* first;
	void* second;
};

int strtoi(char* str, int* result);

int str_to_uint16(const char* str, uint16_t * result);

bool is_directory(char* pathname);

/**
 * _new - initialize an object in the heap calling the constructor with
 *        the varaidic arguments.
 * @obj_name:   the name of the object.
 *
 */
#define new(obj_name, ...) ({                                           \
        struct obj_name* __ptr = malloc(sizeof(struct obj_name));       \
        if (__ptr) {                                                    \
            obj_name##_init(__ptr, ##__VA_ARGS__);                      \
        }                                                               \
        __ptr; })

 /**
  * _destroy -  destroy and free an object from the calling the destructor with
  *             the varaidic arguments.
  * @ptr:       the pointer to the object.
  *
  */
#define delete(ptr, ...) ({                                             \
        int __ret = ptr->__ops_vptr->destroy(ptr);                      \
        if (!__ret) {                                                   \
            free(ptr);                                                  \
        }                                                               \
        __ret; })

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
