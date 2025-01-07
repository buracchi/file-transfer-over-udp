#include "io.h"

#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <unistd.h>

bool file_size_octet(int fd, size_t size[static 1]) {
    struct stat file_stat;
    if (fd < 0) {
        return false;
    }
    if (fstat(fd, &file_stat) < 0) {
        return false;
    }
    switch (file_stat.st_mode & __S_IFMT) {
        case __S_IFBLK:
            uint64_t bytes;
            if (ioctl(fd, BLKGETSIZE64, &bytes) != 0) {
                return false;
            }
            *size = bytes;
            return true;
        case __S_IFREG:
            *size = file_stat.st_size;
            return true;
        default:
            return false;
    }
}

bool file_size_netascii(int fd, size_t size[static 1]) {
    size_t netascii_size = 0;
    size_t bytes_to_read;
    if (!file_size_octet(fd, &bytes_to_read)) {
        return false;
    }
    size_t buffer_size = 4096;
    char *buffer = malloc(buffer_size);
    if (buffer == nullptr) {
        return false;
    }
    while (bytes_to_read) {
        ssize_t bytes_read = read(fd, buffer, buffer_size);
        if (bytes_read == -1) {
            free(buffer);
            return false;
        }
        netascii_size += bytes_read;
        for (size_t i = 0; i < buffer_size; i++) {
            if (buffer[i] == '\n' || buffer[i] == '\r') {
                ++netascii_size;
            }
        }
        bytes_to_read -= bytes_read;
    }
    free(buffer);
    if (lseek(fd, 0, SEEK_SET) == -1) {
        return false;
    }
    *size = netascii_size;
    return true;
}
