#include "session_file.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include <tftp.h>

static inline const char *get_full_path(const char filename[static 1], const char root[static 1]);

static int open_directory_as_memfile(const char filename[static 1],
                                     const char *root,
                                     struct tftp_session_stats_error error[static 1]) {
    const char *dir_path = root == nullptr ? filename : get_full_path(filename, root);
    if (!dir_path) {
        *error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_NOT_DEFINED,
            .error_message = "Could not allocate memory to calculate the directory path.",
        };
        return -1;
    }
    struct __dirstream *dir = opendir(dir_path);
    if (dir == nullptr) {
        *error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_FILE_NOT_FOUND,
            .error_message = "Could not open directory.",
        };
        if (root != nullptr) {
            free((void *)dir_path);
        }
        return -1;
    }
    if (dir_path != filename) {
        free((void *)dir_path);
    }
    size_t size = 0;
    for (struct dirent *entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
        size += strlen(entry->d_name) + 1; // +1 for newline
    }
    rewinddir(dir);
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        *error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_NOT_DEFINED,
            .error_message = "Could not create pipe.",
        };
        return -1;
    }
    if (fcntl(pipefd[0], F_SETPIPE_SZ, size) == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        *error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_NOT_DEFINED,
            .error_message = "Could not set pipe size.",
        };
        return -1;
    }
    for (struct dirent *entry = readdir(dir); entry != nullptr; entry = readdir(dir)) {
        size_t len = strlen(entry->d_name);
        if (write(pipefd[1], entry->d_name, len) != (ssize_t)len
         || write(pipefd[1], "\n", 1) != 1) {
            closedir(dir);
            close(pipefd[0]);
            close(pipefd[1]);
            *error = (struct tftp_session_stats_error) {
                .error_occurred = true,
                .error_number = TFTP_ERROR_NOT_DEFINED,
                .error_message = "Could not write to pipe.",
            };
            return -1;
        }
    }
    closedir(dir);
    close(pipefd[1]);
    return pipefd[0];
}

int session_file_init(const char filename[static 1],
                      const char *root,
                      enum session_file_mode mode,
                      enum tftp_read_type read_type,
                      struct tftp_session_stats_error error[static 1]) {
    int file_descriptor;
    int oflag;
    if (mode == SESSION_FILE_MODE_READ && read_type == TFTP_READ_TYPE_DIRECTORY) {
        return open_directory_as_memfile(filename, root, error);
    }
    switch (mode) {
        case SESSION_FILE_MODE_READ:
            oflag = O_RDONLY;
            break;
        case SESSION_FILE_MODE_WRITE:
            oflag = O_CREAT | O_WRONLY | O_TRUNC;
            break;
        default:
            oflag = O_RDONLY;
    }
    const char *path = root == nullptr ? filename : get_full_path(filename, root);
    if (path == nullptr) {
        *error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = TFTP_ERROR_NOT_DEFINED,
            .error_message = "Could not allocate memory to calculate the file path.",
        };
        return -1;
    }
    errno = 0;
    do {
        file_descriptor = open(path, oflag, 0644);
    } while (file_descriptor == -1 && errno == EINTR);
    if (root != nullptr) {
        free((void *) path);
    }
    if (file_descriptor == -1) {
        enum tftp_error_code error_code;
        char *message;
        switch (errno) {
            case EACCES:
                error_code = TFTP_ERROR_ACCESS_VIOLATION;
                message = "Permission denied.";
                break;
            case ENOENT:
                error_code = TFTP_ERROR_FILE_NOT_FOUND;
                message = "No such file or directory.";
                break;
            default:
                error_code = TFTP_ERROR_NOT_DEFINED;
                message = strerror(errno);
        }
        *error = (struct tftp_session_stats_error) {
            .error_occurred = true,
            .error_number = error_code,
            .error_message = message,
        };
    }
    return file_descriptor;
}

static inline const char *get_full_path(const char filename[static 1], const char root[static 1]) {
    bool is_root_slash_terminated = root[strlen(root) - 1] == '/';
    size_t padding = is_root_slash_terminated ? 0 : 1;
    char *full_path = malloc(strlen(root) + strlen(filename) + padding + 1);
    if (full_path == nullptr) {
        return nullptr;
    }
    strcpy(full_path, root);
    if (!is_root_slash_terminated) {
        strcat(full_path, "/");
    }
    strcat(full_path, filename);
    return full_path;
}
