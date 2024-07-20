#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "Argparsing.h"
#include "FileProcessor.h"
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


// Taken inspiration and adapted from exercises 'Nutzereingaben' and 'File IO' from GRA Week 3
const char* help_msg = "Positional arguments:\n"
                       "   <filename>   The name of the file to be processed\n"
                       "\n"
                       "Optional arguments:\n"
                       "   -c c / --cycles c       The number of cycles used for the simulation (default: c = 100000)\n"
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
                       "   --extended              Calls extended run_simulation-method with additional parameters "
                       "\'policy' and \'use-cache'\n"
                       "   -h / --help             Show this help message and exit\n";


void print_usage(const char* progname) { fprintf(stderr, usage_msg, progname, progname, progname); }

void print_help(const char* progname) { print_usage(progname); fprintf(stderr, "\n%s", help_msg); }

unsigned long check_user_input(char* endptr, char* message, const char* progname, char* option) {
    endptr = NULL;
    long n = strtol(optarg, &endptr, 10);   // Using datatype 'long' to check for negative input
    if (*endptr != '\0' || endptr == optarg) {
        fprintf(stderr, "Invalid input: '%s' is not an int.\n", optarg);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    if (n <= 0 || errno != 0 || n > UINT32_MAX) {
        if (errno == 0) {
            if (n == 0) {   // Allow certain options with value 0
                if (strncmp(option, "--cachelines", 12) == 0) {
                    fprintf(stderr, "Warning: --cachelines must be at least 1. Setting use-cache=n.\n");
                    return 0;
                } else if (strncmp(option, "--cache-latency", 15) == 0 || strncmp(option, "--memory", 8) == 0) {
                    fprintf(stderr, "Warning: A value of 0 for %s is not realistic!\n", option);
                    return 0;
                }
            } else if (n < 0) {  // Negative input not useful for simulation
                fprintf(stderr, "Invalid input: %s\n", message);
            }
        } else if (n > UINT32_MAX) { // Input needs to fit into predefined datatypes for run_simulation method
            fprintf(stderr, "Invalid input: %ld is too big to be converted to an unsigned int.\n", n);
        } else {    // Print errno error message
            fprintf(stderr, "Error parsing number for option %s. %s", option, strerror(errno));
        }
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    return (unsigned)n;
}

int check_cycle_size(int longCycles, const char* progname, struct Configuration* config) {
    if ((!longCycles && !config->callExtended) && config->cycles > INT32_MAX) {
        fprintf(stderr, "Error: Overflow detected. Input for -c/--cycles is too big to be converted "
                        "to an int. Set option --lcycles to increase range.\n");
        print_usage(progname);
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
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
        return "--tf";
    default:
        return "string_data";
    }
}

// Taken from: https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
int is_power_of_two(unsigned long n) { return n && !(n & (n - 1)); }

int is_multiple_of_sixteen(unsigned long n) { return !(n & 15); }


int parse_arguments(int argc, char** argv, struct Configuration* config) {

    const char* progname = argv[0];
    if (argc == 1) {
        fprintf(stderr, "Error: Positional argument missing!\n");
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
    int longCycles = 0; // Default: false

    opterr = 0; // Use own error messages

    while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
        errno = 0;

        switch (opt) {
        case 'c':
            error_msg = "Cycles cannot be smaller than 1.";
            config->cycles = check_user_input(endptr, error_msg, progname, "-c/--cycles");
            // Checked for validity later once we know the range
            break;

        case LONG_CYCLES:
            longCycles = 1;
            config->callExtended = 1;
            break;

        case 'h':
            print_help(progname);
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
            unsigned long s = check_user_input(endptr, error_msg, progname, "--cacheline-size");

            if (!is_multiple_of_sixteen(s)) {
                fprintf(stderr, "Invalid input: Cacheline size should be a multiple of 16 bytes!\n");
                print_usage(progname);
                exit(EXIT_FAILURE);
            } else if (!is_power_of_two(s)) {
                fprintf(stderr, "Invalid input: Cacheline size should be a power of 2!\n");
                print_usage(progname);
                exit(EXIT_FAILURE);
            }

            config->cacheLineSize = s;
            break;

        case CACHELINES:
            error_msg = "Number of cache-lines must be at least 1.";
            unsigned long n = check_user_input(endptr, error_msg, progname, "--cachelines");
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
            error_msg = "Cache-latency cannot be negativ.";
            unsigned long l = check_user_input(endptr, error_msg, progname, "--cache-latency");
            config->cacheLatency = l;
            break;

        case MEMORY_LATENCY:
            error_msg = "Memory-latency cannot be negativ.";
            unsigned long m = check_user_input(endptr, error_msg, progname, "--memory-latency");
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
                fprintf(stderr, "Error: --fifo and --random are both set. Please choose only one option!\n");
                print_usage(progname);
                exit(EXIT_FAILURE);
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
                exit(EXIT_FAILURE);
            }

            config->policy = POLICY_RANDOM;
            break;

        case USE_CACHE: // use-cache=n used for benchmarking
            if (strlen(optarg) < 3 && (strncasecmp(optarg, "n", 2) == 0 || strncasecmp(optarg, "no", 2) == 0)) {
                config->usingCache = 0;
                break;
            } else if (strlen(optarg) < 4 && (strncasecmp(optarg, "y", 2) == 0 || strncasecmp(optarg, "yes", 2) == 0)) {
                break;
            }

            if (*optarg == '\0') {
                fprintf(stderr, "Error: Option --use-cache requires an argument.\n");
            } else {
                fprintf(stderr, "Error: '%s' is not a valid option for --use-cache.\n", optarg);
            }
            print_usage(progname);
            exit(EXIT_FAILURE);

        case TRACEFILE:
            if (*optarg == '\0') {
                fprintf(stderr, "Error: Option --tf requires an argument.\n");
                print_usage(progname);
                exit(EXIT_FAILURE);
            }
            config->tracefile = optarg;
            break;

        case CALL_EXTENDED:
            config->callExtended = 1;
            break;

        case '?':
            option = get_option();
            if (strcmp(option, "string_data") != 0) { // Check if optarg is positional argument
                fprintf(stderr, "Error: Option %s requires an argument.\n", option);
            } else if (argv[optind - 1] != NULL) {
                fprintf(stderr, "Error: Not a valid option '%s'.\n", argv[optind - 1]);
            } else {
                fprintf(stderr, "Error: Not a valid option.\n");
            }

        default:
            print_usage(progname);
            exit(EXIT_FAILURE);
        }
    }

    if (config->memoryLatency < config->cacheLatency) {
        fprintf(stderr, "Warning: Memory latency is less than cache latency.\n");
    }

    check_cycle_size(longCycles, progname, config);

    // Check for Positional Argument
    if (optind < argc) {
        // Check input file for valid file format and save data to requests
        FILE* file = check_file(progname, argv[optind]);
        extract_file_data(progname, file, config);
    } else {
        fprintf(stderr, "Error: Positional argument is missing!\n");
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
