#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Argparsing.h"
#include "FileDataExtraction.h"
#include "Request.h"
#include "Simulation/Policy/Policy.h"

#define CACHE_LATENCY 128
#define CACHELINE_SIZE 129
#define CACHELINES 130
#define CALL_EXTENDED 131
#define DIRECTMAPPED 132
#define FIRST_IN_FIRST_OUT 133
#define FULLASSOCIATIVE 134
#define LEAST_RECENTLY_USED 135
#define LONG_CYCLES 136
#define MEMORY_LATENCY 137
#define RANDOM_CHOICE 138
#define TRACEFILE 139
#define USE_CACHE 140


const char* help_msg = "Positional arguments:\n"
                       "   <filename>   The name of the file to be processed\n"
                       "\n"
                       "Optional arguments:\n"
                       "   -c c / --cycles c       The number of cycles used for the simulation (default: c = 1000)\n"
                       "   --lcycles               If set input for cycles of up to 2^32-1 are allowed\n"
                       "   --directmapped          Simulates a direct-mapped cache\n"
                       "   --fullassociative       Simulates a fully associative cache (Set as default)\n"
                       "   --cacheline-size s      The size of a cache line in bytes (default: 64)\n"
                       "   --cachelines n          The number of cache lines (default: 256)\n"
                       "   --cache-latency l       The cache latency in cycles (default: 2)\n"
                       "   --memory-latency m      The memory latency in cycles (default: 100)\n"
                       "   --lru                   Use LRU as cache-replacement policy (Set as default)\n"
                       "   --fifo                  Use FIFO as cache-replacement policy\n"
                       "   --random                Use random cache-replacement policy\n"
                       "   --use-cache=<Y,n>       Simulates a system with cache or no cache (default: Y)\n"
                       "   --tf=<filename>         The name for a trace file containing all signals. If not set, no "
                       "trace file will be created\n"
                       "   --extended              Calls extended run_simulation-method with additional parameters\n"
                       "   -h / --help             Show this help message and exit\n";

// Taken and adapted from GRA Week 3 "Nutzereingaben" and "File IO"
void print_usage(const char* progname) { fprintf(stderr, usage_msg, progname, progname, progname); }

void print_help(const char* progname) { print_usage(progname); fprintf(stderr, "\n%s", help_msg); }

FILE* check_file(const char* progname, const char* filename_1, const char* filename_2, char* filetype) {
    const char* filename = filename_1;
    FILE* file = fopen(filename, "r");
    if (file == NULL) { // Accept positional argument as first and last command line argument
        file = fopen(filename_2, "r");
        filename = filename_2;
    }

    if (file == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", filetype, strerror(errno));
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    // Lines 71-81 taken and adapted from GRA Week 3 "File IO" files.c
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }
    if (S_ISDIR(file_info.st_mode)) {
        fprintf(stderr, "Error: Filename should not be a directory.\n");
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }
    // Taken from https://stackoverflow.com/questions/5899497/how-can-i-check-the-extension-of-a-file
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        fprintf(stderr, "Error: %s is not a valid file\n", filename);
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    } else if (strcmp(dot+1, "csv") != 0) {
        fprintf(stderr, "Error: %s is not a valid csv file!\n", filename);
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    if (!S_ISREG(file_info.st_mode)) {
        fprintf(stderr, "Error: %s is not a regular file\n", filename);
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }
    return file;
}

unsigned long check_user_input(char* endptr, char* message, const char* progname, char* option,
                               struct Request* requests) {
    endptr = NULL;
    long n = strtol(optarg, &endptr, 10);   // Using datatype 'long' to check for negative input
    if (*endptr != '\0' || endptr == optarg) {
        fprintf(stderr, "Invalid input: '%s' is not a number.\n", optarg);
        print_usage(progname);
        free(requests);
        requests = NULL;
        exit(EXIT_FAILURE);
    }

    if (n <= 0 || errno != 0 || n > UINT32_MAX) {
        if (errno == 0 && n <= 0) { // Negative input and 0 not useful for simulation
            if (n == 0 && strcmp(option, "--cachelines") == 0) {
                fprintf(stderr, "Warning: --cachelines must be at least 1. Setting use-cache=n.\n");
                return 0;
            }
            fprintf(stderr, "Invalid input: %s\n", message);
        } else if (n > UINT32_MAX) { // Input needs to fit into predefined datatypes for run_simulation method
            fprintf(stderr, "Invalid input: %ld is too big to be converted to an unsigned int.\n", n);
        } else {
            fprintf(stderr, "Error parsing number for option %s. %s", option, strerror(errno));
        }
        print_usage(progname);
        free(requests);
        requests = NULL;
        exit(EXIT_FAILURE);
    }
    return (unsigned)n;
}

void check_cycle_size(int longCycles, unsigned int cycles, struct Request* requests, const char* progname, struct Configuration* config) {
    if ((!longCycles && !config->callExtended) && cycles > INT32_MAX) {
        fprintf(stderr, "Error: %d is too big to be converted to an int. "
                        "Set option --lcycles to increase range.\n", cycles);
        print_usage(progname);
        free(requests);
        requests = NULL;
        exit(EXIT_FAILURE);
    }
}

char* get_option() {
    switch (optopt) {
    case 'c':
        return "-c/--cycles";
    case CACHELINE_SIZE:
        return "--cacheline-size";
    case CACHELINES:
        return "--cachelines";
    case CACHE_LATENCY:
        return "--cache-latency";
    case MEMORY_LATENCY:
        return "--memory-latency";
    case USE_CACHE:
        return "--use-cache";
    case TRACEFILE:
        return "--tf=";
    default:
        return "invalid";
    }
}

// Taken from: https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
int is_power_of_two(unsigned long n) { return n && !(n & (n - 1)); }

int is_power_of_sixteen(unsigned long n) { return n && !(n & (n - 4)); }


int parse_arguments(int argc, char** argv, struct Configuration* config) {

    const char* progname = argv[0];
    if (argc == 1) {
        fprintf(stderr, "Positional argument missing!\n");
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    // Set default values for fullassociative cache and run_simulation_extended
    config->cycles = 100000;
    config->directMapped = 0;       // 0 => fullassociative, x => directmapped
    config->cacheLines = 256;
    config->cacheLineSize = 64;
    config->cacheLatency = 2;
    config->memoryLatency = 100;
    config->tracefile = NULL;

    config->policy = POLICY_LRU;    // 0 => lru, 1 => fifo, 2 => random
    config->usingCache = 1;         // Default: true
    config->callExtended = 0;       // Default: false

    // Check input file and save file data to requests
    FILE* file = check_file(progname, argv[argc - 1], argv[1], "input file");
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        fclose(file);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    config->requests = (struct Request*)malloc(sizeof(struct Request) * file_info.st_size);
    config->numRequests = 0;
    if (config->requests == NULL) {
        perror("Error allocating memory buffer for file");
        fclose(file);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    extract_file_data(progname, file, config->requests, &config->numRequests); // Save data to requests

    // Command line argument parsing
    int opt;
    int option_index;
    char* endptr = NULL;
    const char* optstring = "c:h";
    static struct option long_options[] = {{"cycles", required_argument, 0, 'c'},
                                           {"lcycles", no_argument, 0, LONG_CYCLES},
                                           {"directmapped", no_argument, 0, DIRECTMAPPED},
                                           {"fullassociative", no_argument, 0, FULLASSOCIATIVE},
                                           {"cacheline-size", required_argument, 0, CACHELINE_SIZE},
                                           {"cachelines", required_argument, 0, CACHELINES},
                                           {"cache-latency", required_argument, 0, CACHE_LATENCY},
                                           {"memory-latency", required_argument, 0, MEMORY_LATENCY},
                                           {"lru", no_argument, 0, LEAST_RECENTLY_USED},
                                           {"fifo", no_argument, 0, FIRST_IN_FIRST_OUT},
                                           {"random", no_argument, 0, RANDOM_CHOICE},
                                           {"use-cache", required_argument, 0, USE_CACHE},
                                           {"extended", no_argument, 0, CALL_EXTENDED},
                                           {"tf=", required_argument, 0, TRACEFILE},
                                           {"help", no_argument, 0, 'h'},
                                           {0, 0, 0, 0}};

    // Variables needed for parsing
    char* option;
    char* error_msg;
    int isLruSet = 0;
    int isFullassociativeSet = 0;
    int longCycles = 0; // 0 => false, x => true

    opterr = 0; // Use own error messages

    while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
        errno = 0;

        switch (opt) {
        case 'c':
            error_msg = "Cycles cannot be smaller than 1.\n";
            config->cycles = check_user_input(endptr, error_msg, progname, "-c/--cycles", config->requests);
            // to be checked for validity later once we know the range
            break;

        case LONG_CYCLES:
            longCycles = 1;
            config->callExtended = 1;
            break;

        case 'h':
            print_help(progname);
            free(config->requests);
            config->requests = NULL;
            exit(EXIT_SUCCESS);

        case DIRECTMAPPED:
            if (isFullassociativeSet) {
                fprintf(stderr, "Warning: --fullassociative and --directmapped are both set. "
                                "Using default value fullassociative!\n");
                break;
            }
            config->directMapped = 1;
            break;

        case FULLASSOCIATIVE:
            if (config->directMapped) {
                fprintf(stderr, "Warning: --fullassociative and --directmapped are both set. "
                                "Using default value fullassociative!\n");
            }
            isFullassociativeSet = 1;
            break;

        case CACHELINE_SIZE:
            error_msg = "Cacheline size should be at least 1.";
            unsigned long s = check_user_input(endptr, error_msg, progname, "--cacheline-size", config->requests);

            if (!is_power_of_sixteen(s)) {
                fprintf(stderr, "Invalid Input: Cacheline size should be a multiple of 16 bytes!\n");
                print_usage(progname);
                return EXIT_FAILURE;
            }

            config->cacheLineSize = s;
            break;

        case CACHELINES:
            error_msg = "Number of cache-lines must be at least 1.";
            unsigned long n = check_user_input(endptr, error_msg, progname, "--cachelines", config->requests);
            if (n == 0) { // Use no cache for simulation due to user input --cachelines 0
                config->usingCache = 0;
                config->cacheLines = 0;
                break;
            }

            if (!is_power_of_two(n)) {
                fprintf(stderr, "Warning: Number of cachelines are usually a power of two!\n");
            }
            config->cacheLines = n;
            break;

        case CACHE_LATENCY:
            error_msg = "Cache-latency cannot be zero or negativ.";
            unsigned long l = check_user_input(endptr, error_msg, progname, "--cache-latency", config->requests);
            config->cacheLatency = l;
            break;

        case MEMORY_LATENCY:
            error_msg = "Memory-latency cannot be zero or negativ.";
            unsigned long m = check_user_input(endptr, error_msg, progname, "--memory-latency", config->requests);
            config->memoryLatency = m;
            break;

        case LEAST_RECENTLY_USED:
            if (config->policy == POLICY_FIFO || config->policy == POLICY_RANDOM) {
                fprintf(stderr, "Warning: More than one policy set. "
                                "Simulating cache using default value LRU!\n");
            }
            config->policy = POLICY_LRU;
            isLruSet = 1;
            break;

        case FIRST_IN_FIRST_OUT:
            if (isLruSet) {
                fprintf(stderr, "Warning: More than one policy set. Simulating cache using default value LRU!\n");
                break;
            } else if (config->policy == POLICY_RANDOM) {
                fprintf(stderr, "Error: --random and --fifo are both set. Please choose only one option!\n");
                print_usage(progname);
                return EXIT_FAILURE;
            }
            config->policy = POLICY_FIFO;
            break;

        case RANDOM_CHOICE:
            if (isLruSet) {
                fprintf(stderr, "Warning: More than one policy set. Simulating cache using default value LRU!\n");
                break;
            } else if (config->policy == POLICY_FIFO) {
                fprintf(stderr, "Error: --fifo and --random are both set. Please choose only one option!\n");
                print_usage(progname);
                return EXIT_FAILURE;
            }

            config->policy = POLICY_RANDOM;
            break;

        case USE_CACHE:
            if ((strcmp(optarg, "n") == 0) || (strcmp(optarg, "N") == 0) || (strcmp(optarg, "no") == 0) ||
                (strcmp(optarg, "No") == 0)) {
                config->usingCache = 0;
                break;
            } else if ((strcmp(optarg, "y") == 0) || (strcmp(optarg, "Y") == 0) || (strcmp(optarg, "yes") == 0) ||
                       (strcmp(optarg, "Yes") == 0)) {
                break;
            }
            if (optarg == NULL) {
                fprintf(stderr, "Warning: Option --use-cache is not set. Using default value 'Y'.");
            } else {
                fprintf(stderr, "Warning: Not a valid option for --use-cache. Using default value 'Y'.");
            }
            break;

        case TRACEFILE:
            if (optarg == NULL || *optarg == '\0') {
                fprintf(stderr, "Error: Option --tf= requires an argument.\n");
                print_usage(progname);
                return EXIT_FAILURE;
            }
            config->tracefile = optarg;
            break;

        case CALL_EXTENDED:
            config->callExtended = 1;
            break;

        case '?':
            option = get_option();
            if (strcmp(option, "invalid") == 0) {
                fprintf(stderr, "Error: Not a valid argument '%s'!\n", argv[optind - 1]);
            } else {
                //fprintf(stderr, "%s", optarg);
                fprintf(stderr, "Error: Option '%s' requires an argument.\n", option);
            }

        default:
            print_usage(progname);
            return EXIT_FAILURE;
        }
    }

    if (config->memoryLatency < config->cacheLatency) {
        fprintf(stderr, "Warning: Memory latency is less than cache latency.\n");
    }

    check_cycle_size(longCycles, config->cycles, config->requests, progname, config);

    return EXIT_SUCCESS;
}