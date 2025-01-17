#include "server_cli.h"

#include <cstring>
#include <filesystem>
#include <print>
#include <thread>

#include "cli.hpp"
#include <CLI/CLI.hpp>

/*
The Trivial File Transfer Protocol (TFTP) provides a simple, lightweight solution for transferring files over a network using UDP.
It is commonly used for tasks like bootstrapping devices or transferring configuration files in embedded systems.
TFTP operates without the need for authentication or complex features, focusing instead on ease of use and low overhead.
This server implementation supports both IPv4 and IPv6 addresses and a variety of configuration options to
 customize its behavior, such as setting the number of worker threads, managing connection limits, and simulating
 network conditions for testing purposes.
TFTP is inherently insecure as it lacks encryption and authentication mechanisms.
It is recommended to use TFTP in trusted, isolated networks and avoid transferring sensitive data.
The TFTP protocol support a maximum number of 65535 simultaneous connections.
For more details, refer to RFC1350.
*/

namespace TFTP::Server {
    
    class CLIParser final : public TFTP::CLIParser {
    public:
        explicit CLIParser(const std::shared_ptr<cli_args>& args) : TFTP::CLIParser("TFTP Server") {
            const auto processor_count = std::thread::hardware_concurrency();
            const auto default_workers = (processor_count > 1) ? processor_count - 1 : 1;
            const auto max_worker_sessions = std::min(32u, (65535 / default_workers) + (65535 % default_workers != 0));
            
            const std::string BasicOptionsStr = "Basic Options";
            const std::string NetworkSettingsStr = "Network Settings";
            const std::string DebuggingAndSimulationStr = "Debugging and Simulation";
            
            get_option("--help")->group(BasicOptionsStr);
            add_option("directory", root, "Specify root directory for file storage")
                ->group(BasicOptionsStr)
                ->default_val("current directory")
                ->check(CLI::ExistingDirectory)
                ->option_text("ROOT");
            add_flag("--enable-write-requests", args->enable_write_requests, "Enable write requests")
                ->default_val(false)
                ->default_str("")
                ->group(BasicOptionsStr);
            add_flag("--enable-list-requests", args->enable_list_requests, "Enable list requests")
                ->default_val(false)
                ->default_str("")
                ->group(BasicOptionsStr);
            add_flag("--enable-adaptive-timeout", args->enable_adaptive_timeout, "Enable adaptive timeout based on network delays option")
                ->default_val(false)
                ->default_str("")
                ->group(BasicOptionsStr);
            add_option("-w,--workers", args->workers, "Set the number of worker threads")
                ->group(BasicOptionsStr)
                ->default_val(std::to_string(default_workers))
                ->check(CLI::Range(1, 65535))
                ->option_text("COUNT");
            add_option("-m,--max-sessions-per-worker", args->max_worker_sessions, "Define max sessions per worker")
                ->group(BasicOptionsStr)
                ->default_val(std::to_string(max_worker_sessions))
                ->check(CLI::Range(1, 65535))
                ->option_text("SESSIONS");
            
            // Network Settings Group
            add_option("-H,--host", host, "Address to bind the server to")
                ->group(NetworkSettingsStr)
                ->default_val("::")
                ->option_text("ADDRESS");
            add_option("-p,--port", port, "Port number for server to listen on")
                ->group(NetworkSettingsStr)
                ->default_val("6969")
                ->check(CLI::Range(0, 65535))
                ->option_text("PORT");
            add_option("-r,--retries", args->retries, "Number of retransmissions attempts before giving up")
                ->group(NetworkSettingsStr)
                ->default_val("5")
                ->check(CLI::Range(0, 255))
                ->option_text("RETRIES");
            add_option("-t,--timeout", args->timeout_s, "Timeout in seconds before retransmission")
                ->group(NetworkSettingsStr)
                ->default_val("2")
                ->check(CLI::Range(1, 255))
                ->option_text("SECONDS");
            
            // Debugging and Simulation Group
            add_option("-l,--loss-probability", args->loss_probability, "Simulated packet loss probability")
                ->group(DebuggingAndSimulationStr)
                ->default_val("0.0")
                ->check(CLI::Range(0.0, 1.0))
                ->option_text("PROBABILITY");
            add_flag("--disable-fixed-seed", args->disable_fixed_seed, "Disable fixed seed for random number generation during packet loss simulation")
                ->group(DebuggingAndSimulationStr);
            add_option("-v,--log-level", args->verbose_level, "Log verbosity level")
                ->group(DebuggingAndSimulationStr)
                ->transform(CLI::CheckedTransformer(log_level_map, CLI::ignore_case))
                ->default_val("info")
                ->option_text("LEVEL")
                ->description("Verbosity logging level");
                
            final_callback([this, args]() {
                args->host = strdup(host.c_str());
                args->port = strdup(port.c_str());
                if (!count("directory")) {
                    const auto root_path = std::filesystem::current_path();
                    if (!std::filesystem::exists(root_path)) {
                        throw std::runtime_error("Root directory does not exist");
                    }
                    root = root_path.string();
                }
                args->root = strdup(root.c_str());
            });
            
            format();
        }
    
    private:
        std::string host;
        std::string port;
        std::string root;
    };
}

bool cli_args_parse(struct cli_args *args, int argc, const char *argv[]) {
    const auto args_ptr = std::shared_ptr<cli_args>(args, [](cli_args*) {
        // Custom deleter does nothing, args memory is manually managed
    });
    static TFTP::Server::CLIParser cli(args_ptr);
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

void cli_args_destroy(struct cli_args *args) {
    free((void *) args->host);
    free((void *) args->port);
    free((void *) args->root);
}
