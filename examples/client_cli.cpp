#include "client_cli.h"

#include <cstring>
#include <map>
#include <print>

#include <CLI/CLI.hpp>

#include "cli.hpp"

static const std::map<std::string, tftp_mode> tftp_mode_map{
    {"netascii", TFTP_MODE_NETASCII},
    {"octet",    TFTP_MODE_OCTET},
};

namespace TFTP::Client {
    class CLIParser final : public TFTP::CLIParser {
    public:
        explicit CLIParser(const std::shared_ptr<cli_args>& args) : TFTP::CLIParser("TFTP Client") {
            add_option("host", host, "IP address or hostname of the server")
                ->option_text("ADDRESS")
                ->required();
            add_option("-p,--port", port, "Port number for the connection")
                ->default_val("6969")
                ->check(CLI::Range(0, 65535))
                ->option_text("PORT");
            add_option("-r,--retries", args->retries, "Number of retries attempts before giving up")
                ->default_val("3")
                ->check(CLI::Range(0, 255))
                ->option_text("RETRIES");
            
            add_option("-t,--timeout", timeout_val, "Timeout in seconds for response")
                ->check(CLI::Range(1, 255))
                ->option_text("SECONDS");
            add_option("-b,--block-size", block_size_val, "Size of the data block for file transfer")
                ->check(CLI::Range(8, 65464))
                ->option_text("BLOCK_SIZE");
            add_option("-w,--window-size", window_size_val, "Dispatch window size")
                ->check(CLI::Range(1, 65535))
                ->option_text("WINDOW_SIZE");
            add_flag("-a,--adaptive-timeout", args->options.adaptive_timeout, "Enable adaptive timeout based on network delays");
            add_flag("--use-tsize", args->options.use_tsize, "Request file size from the server");
            add_option("-l,--loss-probability", args->loss_probability, "Simulated packet loss probability")
                ->default_val("0.0")
                ->check(CLI::Range(0.0, 1.0))
                ->option_text("PROBABILITY");
            add_option("-v,--log-level", args->verbose_level, "Log verbosity level")
                ->transform(CLI::CheckedTransformer(log_level_map, CLI::ignore_case))
                ->default_val("info")
                ->option_text("LEVEL")
                ->description("Verbosity logging level");
            callback([&, args]() {
                args->host = strdup(host.c_str());
                args->port = strdup(port.c_str());
                args->options.timeout_s = (timeout_val == 0) ? nullptr : new uint8_t(timeout_val);
                args->options.block_size = (block_size_val == 0) ? nullptr : new uint16_t(block_size_val);
                args->options.window_size = (window_size_val == 0) ? nullptr : new uint16_t(window_size_val);
            });
            
            require_subcommand();
            
            auto *list_cmd = add_subcommand("list", "List files on a server directory");
            list_cmd->fallthrough(true);
            list_cmd->add_option("directory", filename, "Directory containing the files to list")
                ->option_text("DIRECTORY")
                ->default_val(".");
            list_cmd->add_option("-m,--mode", args->command_args.put.mode, "Transfer mode")
                ->transform(CLI::CheckedTransformer(tftp_mode_map, CLI::ignore_case))
                ->option_text("MODE")
                ->default_val("octet");
            list_cmd->callback([&, args]() {
                args->command = CLIENT_COMMAND_LIST;
                args->command_args.list.directory = strdup(filename.c_str());
                args->command_args.get.output = output.empty() ? nullptr : strdup(output.c_str());
            });
            
            auto *get_cmd = add_subcommand("get", "Download a file from the server");
            get_cmd->fallthrough(true);
            get_cmd->add_option("filename", filename, "File to download")
                ->option_text("FILENAME")
                ->required();
            get_cmd->add_option("-m,--mode", args->command_args.get.mode, "Transfer mode")
                ->transform(CLI::CheckedTransformer(tftp_mode_map, CLI::ignore_case))
                ->option_text("MODE")
                ->default_val("octet");
            get_cmd->add_option("-o,--output", output, "Output file name")
                ->option_text("OUTPUT_FILE");
            get_cmd->callback([&, args]() {
                args->command = CLIENT_COMMAND_GET;
                args->command_args.get.filename = strdup(filename.c_str());
                args->command_args.get.output = output.empty() ? nullptr : strdup(output.c_str());
            });
            
            auto *put_cmd = add_subcommand("put", "Upload a file to the server");
            put_cmd->fallthrough(true);
            put_cmd->add_option("filename", filename, "File to upload")
                ->option_text("FILENAME")
                ->check(CLI::ExistingFile)
                ->required();
            put_cmd->add_option("-m,--mode", args->command_args.put.mode, "Transfer mode")
                ->transform(CLI::CheckedTransformer(tftp_mode_map, CLI::ignore_case))
                ->option_text("MODE")
                ->default_val("octet");
            put_cmd->callback([&, args]() {
                args->command = CLIENT_COMMAND_PUT;
                args->command_args.put.filename = strdup(filename.c_str());
            });
            
            format();
        }
    
    private:
        std::string host;
        std::string port;
        std::string filename;
        std::string output;
        int timeout_val = 0;
        int block_size_val = 0;
        int window_size_val = 0;
    };
}

bool cli_args_parse(cli_args *args, int argc, const char *argv[]) {
    const auto args_ptr = std::shared_ptr<cli_args>(args, [](cli_args*) {
        // Custom deleter does nothing, args memory is manually managed
    });
    static TFTP::Client::CLIParser cli(args_ptr);
    try {
        cli.parse(argc, argv);
    }
    catch (const CLI::ParseError &e) {
        cli.exit(e);
        return false;
    }
    catch (const std::runtime_error &e) {
        std::println(stderr, "Error: {}", e.what());
        return false;
    }
    return true;
}

void cli_args_destroy(cli_args *args) {
    free((void *) args->host);
    free((void *) args->port);
    if (args->command == CLIENT_COMMAND_GET) {
        free((void *) args->command_args.get.filename);
        free((void *) args->command_args.get.output);
    }
    else if (args->command == CLIENT_COMMAND_PUT) {
        free((void *) args->command_args.put.filename);
    }
    else if (args->command == CLIENT_COMMAND_LIST) {
        free((void *) args->command_args.list.directory);
    }
    delete args->options.timeout_s;
    delete args->options.block_size;
    delete args->options.window_size;
}
