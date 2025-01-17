#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <time.h>

#include <buracchi/tftp/client.h>
#include <logger.h>

#include "client_cli.h"
#include "client_packet_loss.h"

static inline double timespec_to_double(struct timespec t) {
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

static struct tftp_client_response get(struct logger logger[static 1],
                                       uint8_t retries,
                                       const char host[static 1],
                                       const char port[static 1],
                                       const char filename[static 1],
                                       enum tftp_mode mode,
                                       struct tftp_client_options *options,
                                       const char *output_path);

static struct tftp_client_response put(struct logger logger[static 1],
                                       uint8_t retries,
                                       const char host[static 1],
                                       const char port[static 1],
                                       const char filename[static 1],
                                       enum tftp_mode mode,
                                       struct tftp_client_options *options,
                                       const char *input_path);

static void print_stats(struct tftp_client_stats stats[static 1], enum command command, struct logger logger[static 1]);

static bool copy_file_contents(FILE dest[static 1], FILE src[static 1], struct logger logger[static 1]);

int main(int argc, char *argv[static argc + 1]) {
    struct cli_args args;
    struct logger logger;
    if (!cli_args_parse(&args, argc, (const char **) argv)) {
        return EXIT_FAILURE;
    }
    if (!logger_init(&logger, logger_default_config)) {
        return EXIT_FAILURE;
    }
    logger.config.default_level = args.verbose_level;
    auto options = (struct tftp_client_options) {
        .timeout_s = args.options.timeout_s,
        .block_size = args.options.block_size,
        .window_size = args.options.window_size,
        .use_tsize = args.options.use_tsize,
        .use_adaptive_timeout = args.options.adaptive_timeout,
        .is_read_type_list = args.command == CLIENT_COMMAND_LIST,
    };
    struct tftp_client_response response = {};
    bool is_retry = false;
    packet_loss_init(&logger, args.loss_probability, args.disable_fixed_seed);
    do {
        struct tftp_client_options *options_ptr = is_retry ? nullptr : &options;
        if (is_retry) {
            logger_log_warn(&logger, "Server may not support options. Retrying without options.");
        }
        switch (args.command) {
            case CLIENT_COMMAND_LIST:
                response = tftp_client_read(&logger,
                                            args.retries,
                                            args.host,
                                            args.port,
                                            args.command_args.list.directory,
                                            args.command_args.list.mode,
                                            &options,
                                            stdout);
                break;
            case CLIENT_COMMAND_GET:
                response = get(&logger,
                               args.retries,
                               args.host,
                               args.port,
                               args.command_args.get.filename,
                               args.command_args.get.mode,
                               options_ptr,
                               args.command_args.get.output);
                break;
            case CLIENT_COMMAND_PUT:
                response = put(&logger,
                               args.retries,
                               args.host,
                               args.port,
                               args.command_args.put.filename,
                               args.command_args.put.mode,
                               options_ptr,
                               nullptr);
                break;
        }
        if (!response.is_success && (args.command == CLIENT_COMMAND_LIST || !response.error.server_may_not_support_options)) {
            cli_args_destroy(&args);
            logger_destroy(&logger);
            return EXIT_FAILURE;
        }
        if (is_retry) {
            break;
        }
        is_retry = true;
    } while (!response.is_success);
    print_stats(&response.value, args.command, &logger);
    logger_log_debug(&logger, "Closing connection.");
    cli_args_destroy(&args);
    logger_destroy(&logger);
    return EXIT_SUCCESS;
}

static struct tftp_client_response get(struct logger logger[static 1],
                                       uint8_t retries,
                                       const char host[static 1],
                                       const char port[static 1],
                                       const char filename[static 1],
                                       enum tftp_mode mode,
                                       struct tftp_client_options *options,
                                       const char *output_path) {
    FILE *output;
    if (output_path == nullptr) {
        output_path = filename;
    }
    const int fd = open(output_path, O_CREAT | O_WRONLY | O_EXCL, S_IRUSR | S_IWUSR);
    const bool output_file_already_exist = (fd == -1 && errno == EEXIST);
    if (fd == -1 && !output_file_already_exist) {
        logger_log_error(logger, "Could not open file %s for writing. %s", output_path, strerror(errno));
        goto fail;
    }
    if (output_file_already_exist) {
        output = fopen(output_path, "wb");
    }
    else {
        output = fdopen(fd, "wb");
    }
    if (output == nullptr) {
        logger_log_error(logger, "Could not open file %s for writing. %s", output_path, strerror(errno));
        goto fail;
    }
    FILE *tmp = tmpfile();
    if (tmp == nullptr) {
        logger_log_error(logger, "Could not create temporary file. %s", strerror(errno));
        fclose(output);
        goto fail;
    }
    auto response = tftp_client_read(logger, retries, host, port, filename, mode, options, tmp);
    if (!response.is_success) {
        goto fail2;
    }
    if (!copy_file_contents(output, tmp, logger)) {
        logger_log_error(logger, "Failed to copy the temporary file contents into the output file.");
        response.is_success = false;
        response.error.server_may_not_support_options = false;
        goto fail2;
    }
    if (fclose(tmp) == EOF) {
        logger_log_warn(logger, "Failed to close the temporary file. %s", strerror(errno));
    }
    if (fclose(output) == EOF) {
        logger_log_warn(logger, "Failed to close the output file. %s", strerror(errno));
    }
    return response;
fail2:
    fclose(tmp);
    fclose(output);
    if (!output_file_already_exist) {
        if (remove(output_path) == -1) {
            logger_log_warn(logger, "Failed to remove the output file. %s", strerror(errno));
        }
    }
    return response;
fail:
    return (struct tftp_client_response) {
        .is_success = false,
        .error.server_may_not_support_options = false,
    };
}

static struct tftp_client_response put(struct logger logger[static 1],
                                       uint8_t retries,
                                       const char host[static 1],
                                       const char port[static 1],
                                       const char filename[static 1],
                                       enum tftp_mode mode,
                                       struct tftp_client_options *options,
                                       const char *input_path) {
    if (input_path == nullptr) {
        input_path = filename;
    }
    FILE *input = fopen(input_path, "rb");
    if (input == nullptr) {
        logger_log_error(logger, "Could not open file %s for reading. %s", input_path, strerror(errno));
        return (struct tftp_client_response) {
            .is_success = false,
            .error.server_may_not_support_options = false,
        };
    }
    auto const response = tftp_client_write(logger, retries, host, port, filename, mode, options, input);
    if (fclose(input) == EOF) {
        logger_log_warn(logger, "Failed to close the input file. %s", strerror(errno));
    }
    return response;
}

static void print_stats(struct tftp_client_stats stats[static 1], enum command command, struct logger logger[static 1]) {
    struct timespec end_time;
    bool end_time_error_occurred = clock_gettime(CLOCK_MONOTONIC, &end_time) == -1;
    if (clock_gettime(CLOCK_MONOTONIC, &end_time) == -1) {
        logger_log_warn(logger, "Failed to get end time. %s", strerror(errno));
    }
    if (stats->start_time.error_occurred || end_time_error_occurred) {
        logger_log_warn(logger, "Could not calculate transfer duration.");
        return;
    }
    const double duration = timespec_to_double(end_time) - timespec_to_double(stats->start_time.value);
    static constexpr const int bits_per_byte = 8;
    static constexpr const int unit_step = 1000;
    static const char *unit_prefix[] = {"", "K", "M", "G", "T", "P", "E", "Z", "Y"};
    const double bit_speed = (double) stats->file_bytes_transferred * bits_per_byte / duration;
    const double byte_speed = (double) stats->file_bytes_transferred / duration;
    double display_speed = byte_speed;
    size_t prefix_index = 0;
    while (display_speed > unit_step && prefix_index < (sizeof unit_prefix / sizeof *unit_prefix)) {
        display_speed /= unit_step;
        prefix_index++;
    }
    logger_log_info(logger, "%s %zu bytes in %.3f seconds [%.2f %sB/s, %.0f bit/s]",
                    command == CLIENT_COMMAND_PUT ? "Sent" : "Received",
                    stats->file_bytes_transferred,
                    duration,
                    display_speed,
                    unit_prefix[prefix_index],
                    bit_speed);
}

static bool copy_file_contents(FILE dest[static 1], FILE src[static 1], struct logger logger[static 1]) {
    if (fflush(src) == EOF) {
        logger_log_error(logger, "Could not flush source file. %s", strerror(errno));
        return false;
    }
    const off64_t file_size = ftello64(src);
    if (file_size < 0) {
        logger_log_error(logger, "Could not get source file size. %s", strerror(errno));
        return false;
    }
    rewind(src);
    const int src_fd = fileno(src);
    if (src_fd < 0) {
        logger_log_error(logger, "Could not get source file descriptor. %s", strerror(errno));
        return false;
    }
    const int dest_fd = fileno(dest);
    if (dest_fd < 0) {
        logger_log_error(logger, "Could not get destination file descriptor. %s", strerror(errno));
        return false;
    }
    off64_t offset = 0;
    while (offset < file_size) {
        if (sendfile64(dest_fd, src_fd, &offset, file_size - offset) == -1) {
            logger_log_error(logger, "Could not copy source file into destination file. %s", strerror(errno));
            return false;
        }
    }
    return true;
}
