#ifndef IO_H
#define IO_H

#include <stddef.h>

bool file_size_octet(int fd, size_t size[static 1]);

bool file_size_netascii(int fd, size_t size[static 1]);

#endif // IO_H
