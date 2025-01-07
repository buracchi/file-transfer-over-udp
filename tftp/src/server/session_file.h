#ifndef SESSION_FILE_H
#define SESSION_FILE_H

#include <buracchi/tftp/server_session_stats.h>

enum session_file_mode {
    SESSION_FILE_MODE_READ,
    SESSION_FILE_MODE_WRITE,
};

int session_file_init(const char filename[static 1],
                      const char *root,
                      enum session_file_mode mode,
                      enum tftp_read_type read_type,
                      struct tftp_session_stats_error error[static 1]);

#endif // SESSION_FILE_H
