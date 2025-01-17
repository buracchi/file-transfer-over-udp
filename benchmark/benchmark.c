#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <math.h>

#include <buracchi/tftp/client.h>
#include <logger.h>
#include <tftp.h>

const char *result_filepath = "benchmark_results.csv";
constexpr int iterations = 10;
constexpr uint8_t retries = 255;
const char *host = "::";
int current_port = 6969;
char port_str[8] = "6969";
uint8_t timeout_val = 1;
uint16_t block_size_val = 1450;

const char *files[] = {"1MB", "10MB", "100MB"};
constexpr int num_files = sizeof(files) / sizeof(files[0]);
constexpr uint16_t window_sizes[] = {1, 2, 4, 8, 16};
constexpr int num_window_sizes = sizeof(window_sizes) / sizeof(window_sizes[0]);
constexpr double loss_rates[] = {0.001, 0.01, 0.1};
constexpr int num_loss_rates = sizeof(loss_rates) / sizeof(loss_rates[0]);

static pid_t start_server(double loss_rate);

static void stop_server(pid_t server_pid);

static bool check_resume_benchmark(int *resume_file,
                                   int *resume_timeout_mode,
                                   int *resume_loss_rate,
                                   int *resume_window_size,
                                   int *completed_iterations);

static int calculate_required_iterations(int file_idx, int timeout_mode, int loss_rate_idx);

static void advance_configuration(int *file_idx,
                                  int *timeout_mode,
                                  int *loss_rate_idx,
                                  int *window_size_idx,
                                  int *completed_iterations);

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[static argc + 1]) {
    pid_t server_pid = -1;
    struct logger logger;
    if (!logger_init(&logger, logger_default_config)) {
        fprintf(stderr, "Failed to initialize logger\n");
        exit(EXIT_FAILURE);
    }
    logger.config.default_level = LOGGER_LOG_LEVEL_OFF;
    
    FILE *tmp = tmpfile();
    if (tmp == nullptr) {
        fprintf(stderr, "Failed to create temporary file\n");
        exit(EXIT_FAILURE);
    }
    
    FILE *result_file;
    int resume_file = 0;
    int resume_timeout_mode = 0;
    int resume_loss_rate = 0;
    int resume_window_size = 0;
    int completed_iterations = 0;
    bool resume_from_existing = check_resume_benchmark(&resume_file, &resume_timeout_mode, &resume_loss_rate,
                                                       &resume_window_size, &completed_iterations);
    
    if (resume_from_existing) {
        result_file = fopen(result_filepath, "a");
        printf("Resuming from previous run: File=%s, Timeout=%s, Window=%d, Loss=%.3f, Completed=%d\n",
               files[resume_file], resume_timeout_mode ? "Adaptive" : "1s",
               window_sizes[resume_window_size], loss_rates[resume_loss_rate], completed_iterations);
    } else {
        result_file = fopen(result_filepath, "w");
    }
    
    if (!result_file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    
    if (!resume_from_existing) {
        fprintf(result_file, "sep=,\nFile Size,Timeout,Window Size,Packet Loss Rate,Transfer Duration\n");
    }
    
    puts("Starting Benchmarks.\n");
    
    for (int f = resume_from_existing ? resume_file : 0; f < num_files; f++) {
        const char *filename = files[f];
        
        for (int timeout_mode = resume_from_existing ? resume_timeout_mode : 1; timeout_mode >= 0; timeout_mode--) {
            const char *timeout_str = (timeout_mode ? "Adaptive" : "1s");
            
            for (int lr = resume_from_existing ? resume_loss_rate : 0; lr < num_loss_rates; lr++) {
                const double loss_rate = loss_rates[lr];
                
                if (server_pid > 0) {
                    stop_server(server_pid);
                    current_port++;
                    snprintf(port_str, sizeof(port_str), "%d", current_port);
                }
                server_pid = start_server(loss_rate);
                
                for (int ws = resume_from_existing ? resume_window_size : 0; ws < num_window_sizes; ws++) {
                    uint16_t current_window_size = window_sizes[ws];
                    
                    // Calculate initial iteration based on resume or default logic
                    int start_iteration;
                    if (resume_from_existing) {
                        start_iteration = completed_iterations;
                        resume_from_existing = false;
                    } else {
                        start_iteration = calculate_required_iterations(f, timeout_mode, lr);
                    }
                    
                    for (int iter = start_iteration; iter <= iterations; iter++) {
                        struct timespec start, end;
                        clock_gettime(CLOCK_MONOTONIC, &start);
                        
                        auto response = tftp_client_read(&logger,
                                                         retries,
                                                         host,
                                                         port_str,
                                                         filename,
                                                         TFTP_MODE_OCTET,
                                                         &(struct tftp_client_options) {
                                                             .timeout_s = &timeout_val,
                                                             .block_size = &block_size_val,
                                                             .window_size = &current_window_size,
                                                             .use_adaptive_timeout = timeout_mode,
                                                         },
                                                         tmp);
                        
                        clock_gettime(CLOCK_MONOTONIC, &end);
                        
                        rewind(tmp);
                        if (!response.is_success) {
                            fprintf(stderr, "Request failed for file %s\n", filename);
                            iter--; // Decrement to retry this iteration
                            continue;
                        }
                        
                        double elapsed = ((double) end.tv_sec + (double) end.tv_nsec / 1e9)
                                         - ((double) start.tv_sec + (double) start.tv_nsec / 1e9);
                        
                        if (iter > 0) {
                            fprintf(result_file, "%s,%s,%d,%.3f,%.3f\n",
                                    filename, timeout_str, current_window_size, loss_rate, elapsed);
                            fflush(result_file);
                            
                            printf("File: %s\tTimeout: %s\tWindow Size: %d\tPacket Loss Rate: %.3f\tDuration: %.3f\n",
                                   filename, timeout_str, current_window_size, loss_rate, elapsed);
                        }
                    }
                }
            }
        }
    }
    
    if (server_pid > 0) {
        stop_server(server_pid);
    }
    
    fclose(tmp);
    logger_destroy(&logger);
    fclose(result_file);
    return EXIT_SUCCESS;
}

static int calculate_required_iterations(int file_idx, int timeout_mode, int loss_rate_idx) {
    if (timeout_mode == 0) {
        if ((file_idx == 1 && loss_rate_idx == 1) || (file_idx == 2 && loss_rate_idx == 0)) {
            return iterations / 2 - 1;
        } else if (file_idx == 2 || (file_idx == 1 && loss_rate_idx == 2)) {
            return iterations;
        }
    }
    return 0;
}

static bool check_resume_benchmark(int *resume_file,
                                   int *resume_timeout_mode,
                                   int *resume_loss_rate,
                                   int *resume_window_size,
                                   int *completed_iterations) {
    FILE *check_file = fopen(result_filepath, "r");
    if (check_file == nullptr) {
        return false;
    }
    
    bool can_resume = false;
    char header1[256], header2[256];
    
    if (fgets(header1, sizeof(header1), check_file) != nullptr &&
        fgets(header2, sizeof(header2), check_file) != nullptr) {
        
        if (strncmp(header1, "sep=,", 5) == 0 &&
            strstr(header2, "File Size,Timeout,Window Size,Packet Loss Rate,Transfer Duration") != nullptr) {
            
            // Read the last line to determine where to resume from
            char line[256];
            char last_line[256] = {0};
            
            while (fgets(line, sizeof(line), check_file) != nullptr) {
                strcpy(last_line, line);
            }
            
            if (last_line[0] != '\0') {
                char file_size[32];
                char timeout_str[32];
                int window_size;
                double loss_rate;
                double duration;
                
                if (sscanf(last_line, "%[^,],%[^,],%d,%lf,%lf",
                           file_size, timeout_str, &window_size, &loss_rate, &duration) == 5) {
                    
                    // Find the indices for resumption
                    for (int i = 0; i < num_files; i++) {
                        if (strcmp(files[i], file_size) == 0) {
                            *resume_file = i;
                            break;
                        }
                    }
                    
                    *resume_timeout_mode = (strcmp(timeout_str, "Adaptive") == 0) ? 1 : 0;
                    
                    for (int i = 0; i < num_window_sizes; i++) {
                        if (window_sizes[i] == window_size) {
                            *resume_window_size = i;
                            break;
                        }
                    }
                    
                    for (int i = 0; i < num_loss_rates; i++) {
                        if (fabs(loss_rates[i] - loss_rate) < 0.0001) {
                            *resume_loss_rate = i;
                            break;
                        }
                    }
                    
                    // Count completed iterations for this test
                    rewind(check_file);
                    fgets(line, sizeof(line), check_file); // Skip sep=,
                    fgets(line, sizeof(line), check_file); // Skip header
                    
                    *completed_iterations = calculate_required_iterations(
                        *resume_file, 
                        *resume_timeout_mode, 
                        *resume_loss_rate
                    );
                    
                    while (fgets(line, sizeof(line), check_file) != nullptr) {
                        char curr_file[32];
                        char curr_timeout[32];
                        int curr_window;
                        double curr_loss;
                        
                        if (sscanf(line, "%[^,],%[^,],%d,%lf",
                                   curr_file, curr_timeout, &curr_window, &curr_loss) == 4) {
                            if (strcmp(curr_file, file_size) == 0 &&
                                strcmp(curr_timeout, timeout_str) == 0 &&
                                curr_window == window_size &&
                                fabs(curr_loss - loss_rate) < 0.0001) {
                                (*completed_iterations)++;
                            }
                        }
                    }
                    
                    // Calculate required iterations for this configuration
                    int required_iterations = calculate_required_iterations(
                        *resume_file,
                        *resume_timeout_mode,
                        *resume_loss_rate
                    );
                    
                    // If all required iterations are completed, advance to the next configuration
                    if (*completed_iterations >= required_iterations) {
                        advance_configuration(
                            resume_file,
                            resume_timeout_mode,
                            resume_loss_rate,
                            resume_window_size,
                            completed_iterations
                        );
                    }
                    
                    can_resume = true;
                }
            }
        }
    }
    
    fclose(check_file);
    return can_resume;
}

static void advance_configuration(int *file_idx,
                                  int *timeout_mode,
                                  int *loss_rate_idx,
                                  int *window_size_idx,
                                  int *completed_iterations) {
    // Try to advance to the next window size first
    if (*window_size_idx < num_window_sizes - 1) {
        (*window_size_idx)++;
    } 
    // If at the last window size, move to the next loss rate
    else if (*loss_rate_idx < num_loss_rates - 1) {
        (*loss_rate_idx)++;
        *window_size_idx = 0;
    } 
    // If at the last loss rate, move to the next timeout mode
    else if (*timeout_mode > 0) {
        (*timeout_mode)--;
        *loss_rate_idx = 0;
        *window_size_idx = 0;
    } 
    // If at the last timeout mode, move to the next file
    else if (*file_idx < num_files - 1) {
        (*file_idx)++;
        *timeout_mode = 1;  // Reset to adaptive timeout
        *loss_rate_idx = 0;
        *window_size_idx = 0;
    }

    // Reset iterations counter for the new configuration
    *completed_iterations = calculate_required_iterations(
        *file_idx, 
        *timeout_mode, 
        *loss_rate_idx
    );
}

static pid_t start_server(double loss_rate) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        // Child process
        char loss_rate_str[8];
        snprintf(loss_rate_str, sizeof(loss_rate_str), "%.3f", loss_rate);
        execl("./server", "server",
              "--enable-adaptive-timeout",
              "-l", loss_rate_str,
              "-r", "255",
              "-v", "warn",
              "-p", port_str,
              nullptr);
        // If execl returns, an error occurred
        perror("execl");
        exit(EXIT_FAILURE);
    }
    
    // Parent process
    printf("Started server process with PID %d on port %s and loss rate %.3f\n", pid, port_str, loss_rate);
    sleep(2); // Wait a few seconds for server to initialize
    return pid;
}

static void stop_server(pid_t server_pid) {
    if (server_pid > 0) {
        printf("Sending SIGINT to server process %d\n", server_pid);
        kill(server_pid, SIGINT);
        
        time_t start_time = time(nullptr);
        
        while (waitpid(server_pid, nullptr, WNOHANG) == 0) {
            if (time(nullptr) - start_time >= 10) {
                printf("Server process did not terminate after 10 seconds. Sending SIGKILL.\n");
                kill(server_pid, SIGKILL);
                continue;
            }
            usleep(100'000);
        }
        
        printf("Server process terminated\n");
        
        // Wait a few seconds before starting the next server
        sleep(2);
    }
}
