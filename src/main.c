#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "Policy/Policy.h"
#include "Request.h"
#include "Result.h"
#include <stdbool.h>

#define DIRECTMAPPED 128
#define FULLASSOCIATIVE 129
#define CACHELINE_SIZE 130
#define CACHELINES 131
#define CACHE_LATENCY 132
#define MEMORY_LATENCY 133
#define LEAST_RECENTLY_USED 134
#define FIRST_IN_FIRST_OUT 135
#define RANDOM_CHOICE 136
#define USE_CACHE 137
#define TRACEFILE 138
#define LONG_CYCLES 139
#define CALL_EXTENDED 140

extern struct Result run_simulation(int cycles, int directMapped, unsigned cacheLines, unsigned cacheLineSize,
                                    unsigned cacheLatency, unsigned memoryLatency, size_t numRequests,
                                    struct Request requests[], const char* tracefile);

extern struct Result run_simulation_extended(uint32_t cycles, int directMapped, unsigned int cacheLines,
                                             unsigned int cacheLineSize, unsigned int cacheLatency,
                                             unsigned int memoryLatency, size_t numRequests, struct Request requests[],
                                             const char* tracefile, enum CacheReplacementPolicy policy, int usingCache);

// Taken and adapted from GRA Week 3 "Nutzereingaben" and "File IO"
const char* usage_msg =
    "usage: %s <filename> [-c c/--cycles c] [--lcycles] [--directmapped] [--fullassociative] "
    "[--cacheline-size s] [--cachelines n] [--cache-latency l] [--memorylatency m] "
    "[--lru] [--fifo] [--random] [--use-cache=<Y,n>] [--tf=<filename>] [--extended] [-h/--help]\n"
    "   -c c / --cycles c       Set the number of cycles to be simulated to c. Allows inputs in range [0,2^16-1]\n"
    "   --lcycles               Allow input of cycles of up to 2^32-1\n"
    "   --directmapped          Simulate a direct-mapped cache\n"
    "   --fullassociative       Simulate a fully associative cache\n"
    "   --cacheline-size s      Set the cache line size to s bytes\n"
    "   --cachelines n          Set the number of cachelines to n\n"
    "   --cache-latency l       Set the cache latency to l cycles\n"
    "   --memory-latency m      Set the memory latency to m cycles\n"
    "   --lru                   Use LRU as cache-replacement policy\n"
    "   --fifo                  Use FIFO as cache-replacement policy\n"
    "   --random                Use random cache-replacement policy\n"
    "   --use-cache=<Y,n>       Simulates a system with cache or no cache\n"
    "   --tf=<filename>         File name for a trace file containing all signals. If not set, no "
    "trace file will be created\n"
    "   -h / --help             Show help message and exit\n";

const char* help_msg = "Positional arguments:\n"
                       "   <filename>   The name of the file to be processed\n"
                       "\n"
                       "Optional arguments:\n"
                       "   -c c / --cycles c       The number of cycles used for the simulation (default: c = 1000)\n"
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
                       "   --extended              Calls extended run_simulation-method with more parameters\n"
                       "   -h / --help             Show this help message and exit\n";

void print_usage(const char* progname) { fprintf(stderr, usage_msg, progname, progname, progname); }

void print_help(const char* progname) { print_usage(progname); fprintf(stderr, "\n%s", help_msg); }

unsigned long check_user_input(char* endptr, char* message, const char* progname, char* option,
                               struct Request* requests) {
    endptr = NULL;
    long n = strtol(optarg, &endptr, 10);   // Using datatype 'long' to check for negative input
    if (*endptr != '\0' || endptr == optarg) {
        fprintf(stderr, "Invalid input: %s is not a number.\n", optarg);
        print_usage(progname);
        free(requests);
        requests = NULL;
        exit(EXIT_FAILURE);
    }

    if (n <= 0 || errno != 0) {
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
    return (unsigned) n;
}

FILE* check_file(const char* progname, const char* filename_1, const char* filename_2, struct Request* requests,
                 char* filetype) {
    const char* filename = filename_1;
    FILE* file = fopen(filename, "r");
    if (file == NULL) { // Accept positional argument as first and last command line argument
        file = fopen(filename_2, "r");
        filename = filename_2;
    }

    if (file == NULL) {
        fprintf(stderr, "Error opening %s: %s\n", filetype, strerror(errno));
        print_usage(progname);
        if (requests != NULL) {
            free(requests);
            requests = NULL;
        }
        exit(EXIT_FAILURE);
    }

    // Lines 138-158 taken and adapted from GRA Week 3 "File IO" files.c
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        fclose(file);
        print_usage(progname);
        if (requests != NULL) {
            free(requests);
            requests = NULL;
        }
        exit(EXIT_FAILURE);
    }
    if (!S_ISREG(file_info.st_mode)) {
        fprintf(stderr, "%s is not a regular file\n", filename);
        fclose(file);
        print_usage(progname);
        if (requests != NULL) {
            free(requests);
            requests = NULL;
        }
        exit(EXIT_FAILURE);
    }
    if (S_ISDIR(file_info.st_mode)) {
        fprintf(stderr, "Filename should not be a directory.\n");
        fclose(file);
        print_usage(progname);
        if (requests != NULL) {
            free(requests);
            requests = NULL;
        }
        exit(EXIT_FAILURE);
    }

    return file;
}

void extract_file_data(const char* progname, FILE* file, struct Request* requests, size_t* numRequests) {
    // Check for invalid file format and save file content to requests
    // Inspired by: https://github.com/portfoliocourses/c-example-code/blob/main/csv_to_struct_array.c
    int read_line;

    do {
        char we;
        uint32_t addr;
        uint32_t data;

        read_line = fscanf(file, "%c,%i,%i\n", &we, &addr, &data);

        if (read_line < 3 && !feof(file)) {
            if (read_line == -1) {
                fprintf(stderr, "Wrong file format! An Error occurred while reading from the file.\n");
                fclose(file);
                print_usage(progname);
                free(requests);
                requests = NULL;
                exit(EXIT_FAILURE);
            }
            if (read_line < 2) { // Address was not read from file
                fprintf(stderr, "Wrong file format! No address given.\n");
                fclose(file);
                print_usage(progname);
                free(requests);
                requests = NULL;
                exit(EXIT_FAILURE);
            }
        }

        if ((we == 'W' || we == 'w') && read_line < 3) { // Write should have data written in the file
            fprintf(stderr, "Wrong file format! No data saved.\n");
            fclose(file);
            print_usage(progname);
            free(requests);
            requests = NULL;
            exit(EXIT_FAILURE);
        }
        if ((we == 'R' || we == 'r') && read_line == 3) { // Read should not have data written in the file
            fprintf(stderr, "Wrong file format! When reading from a file, data should be empty.\n");
            fclose(file);
            print_usage(progname);
            free(requests);
            requests = NULL;
            exit(EXIT_FAILURE);
        }

        requests[*numRequests].addr = addr;
        requests[*numRequests].data = data;

        if (we == 'R' || we == 'r') {
            requests[*numRequests].we = 0;
        } else if (we == 'W' || we == 'w') {
            requests[*numRequests].we = 1;
        } else {
            fprintf(stderr, "'%c' is not a valid operation\n", we);
            fclose(file);
            print_usage(progname);
            free(requests);
            requests = NULL;
            exit(EXIT_FAILURE);
        }

        (*numRequests)++;

        if (ferror(file)) {
            fprintf(stderr, "Error reading from file.\n");
            fclose(file);
            print_usage(progname);
            free(requests);
            requests = NULL;
            exit(EXIT_FAILURE);
        }

    } while (!feof(file));

    fclose(file);
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

void checkCycleSize(bool longCycles, uint32_t cycles, struct Request* requests, const char* progname) {
    if (!longCycles && cycles > INT32_MAX) {
        fprintf(stderr, "Error: %d is too big to be converted to an int. Set option --lcycles to increase range.\n",
                cycles);
        print_usage(progname);
        free(requests);
        requests = NULL;
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char** argv) {

    const char* progname = argv[0];

    if (argc == 1) {
        fprintf(stderr, "Positional argument missing!\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Default values for fullassociative cache
    int directMapped = 0; // 0 => fullassociative, x => directmapped
    unsigned int cycles = 1000;
    unsigned int cacheLines = 256;
    unsigned int cacheLineSize = 64;
    unsigned int cacheLatency = 2;
    unsigned int memoryLatency = 100;
    enum CacheReplacementPolicy policy = POLICY_LRU; // 0 => lru, 1 => fifo, 2 => random
    int usingCache = 1; // 1 => true, x => false
    const char* tracefile = NULL;

    // Extract file data
    FILE* file = check_file(progname, argv[argc - 1], argv[1], NULL, "input file");
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        fclose(file);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    size_t numRequests = 0;
    struct Request* requests = (struct Request*)malloc(sizeof(struct Request) * file_info.st_size);
    if (requests == NULL) {
        perror("Error allocating memory buffer for file");
        fclose(file);
        print_usage(progname);
        free(requests);
        requests = NULL;
        return EXIT_FAILURE;
    }

    extract_file_data(progname, file, requests, &numRequests);

    // PARSING
    int opt;
    int option_index;
    char* endptr = NULL;
    const char* optstring = "c:h";
    static struct option long_options[] = {
        {"cycles", required_argument, 0, 'c'},
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

    FILE* t_file;
    char* error_msg;
    char* option;
    int isFullassociativeSet = 0;
    int isLruSet = 0;
    bool longCycles = false;
    opterr = 0; // Use own error messages

    while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
        errno = 0;

        switch (opt) {
        case 'c':
            error_msg = "Cycles cannot be smaller than 1.";
            cycles = check_user_input(endptr, error_msg, progname, "-c/--cycles", requests);
            // to be checked for validity later once we know the range
            break;

        case LONG_CYCLES:
            longCycles = true;
            break;

        case 'h':
            print_help(progname);
            free(requests);
            requests = NULL;
            return EXIT_SUCCESS;

        case DIRECTMAPPED:
            if (isFullassociativeSet) {
                fprintf(stderr, "Warning: --fullassociative and --directmapped are both set. "
                                "Using default value fullassociative!");
                break;
            }
            directMapped = 1;
            break;

        case FULLASSOCIATIVE:
            if (directMapped) {
                fprintf(stderr, "Warning: --fullassociative and --directmapped are both set. "
                                "Using default value fullassociative!");
            }
            isFullassociativeSet = 1;
            break;

        case CACHELINE_SIZE:
            error_msg = "Cacheline size should be at least 1.";
            unsigned long s = check_user_input(endptr, error_msg, progname, "--cacheline-size", requests);

            if (!is_power_of_sixteen(s)) {
                fprintf(stderr, "Invalid Input: Cacheline size should be a multiple of 16 bytes!\n");
                print_usage(progname);
                free(requests);
                requests = NULL;
                return EXIT_FAILURE;
            }

            cacheLineSize = s;
            break;

        case CACHELINES:
            error_msg = "Number of cache-lines must be at least 1.";
            unsigned long n = check_user_input(endptr, error_msg, progname, "--cachelines", requests);
            if (n == 0) { // Use no cache for simulation due to user input --cachelines 0
                usingCache = 0;
                cacheLines = 0;
                break;
            }

            if (!is_power_of_two(n)) {
                fprintf(stderr, "Warning: Number of cachelines are usually a power of two!\n");
            }
            cacheLines = n;
            break;

        case CACHE_LATENCY:
            error_msg = "Cache-latency cannot be zero or negativ.";
            unsigned long l = check_user_input(endptr, error_msg, progname, "--cache-latency", requests);
            cacheLatency = l;
            break;

        case MEMORY_LATENCY:
            error_msg = "Memory-latency cannot be zero or negativ.";
            unsigned long m = check_user_input(endptr, error_msg, progname, "--memory-latency", requests);
            memoryLatency = m;
            break;

        case LEAST_RECENTLY_USED:
            if (policy == POLICY_FIFO || policy == POLICY_RANDOM) {
                fprintf(stderr, "Warning: More than one policy set. "
                                "Simulating cache using default value LRU!\n");
            }
            policy = POLICY_LRU;
            isLruSet = 1;
            break;

        case FIRST_IN_FIRST_OUT:
            if (isLruSet) {
                fprintf(stderr,
                        "Warning: More than one policy set. Simulating cache using default value LRU!\n");
                break;
            } else if (policy == POLICY_RANDOM) {
                fprintf(stderr,
                        "Error: --random and --fifo are both set. Please choose only one option!\n");
                print_usage(progname);
                free(requests);
                requests = NULL;
                return EXIT_FAILURE;
            }
            policy = POLICY_FIFO;
            break;

        case RANDOM_CHOICE:
            if (isLruSet) {
                fprintf(stderr,
                        "Warning: More than one policy set. Simulating cache using default value LRU!\n");
                break;
            } else if (policy == POLICY_FIFO) {
                fprintf(stderr,
                        "Error: --fifo and --random are both set. Please choose only one option!\n");
                print_usage(progname);
                free(requests);
                requests = NULL;
                return EXIT_FAILURE;
            }

            policy = POLICY_RANDOM;
            break;

        case USE_CACHE:
            if ((strcmp(optarg, "n") == 0) || (strcmp(optarg, "N") == 0) || (strcmp(optarg, "no") == 0) ||
                (strcmp(optarg, "No") == 0)) {
                usingCache = 0;
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
            t_file = check_file(progname, optarg, NULL, requests, "tracefile");
            tracefile = optarg;
            fclose(t_file);
            break;

        case '?':
            option = get_option();
            if (strcmp(option, "invalid") == 0) {
                fprintf(stderr, "Error: Not a valid argument '%s'!\n", argv[optind - 1]);
                print_usage(progname);
                free(requests);
                requests = NULL;
                exit(EXIT_FAILURE);
            }
            fprintf(stderr, "Error: Option '%s' requires an argument.\n", option);

        default:
            print_usage(progname);
            free(requests);
            requests = NULL;
            return EXIT_FAILURE;
        }
    }

    // TODO: Weitere nicht-sinnvolle inputs abfangen
    if (memoryLatency < cacheLatency) {
        fprintf(stderr, "Warning: Memory latency is less than cache latency.\n");
    } // TODO: Cycles macht keinen Sinn

    checkCycleSize(longCycles, cycles, requests, progname);

    struct Result result = run_simulation_extended(cycles, directMapped, cacheLines, cacheLineSize, cacheLatency,
                                                   memoryLatency, numRequests, requests, tracefile, policy, usingCache);

    free(requests);
    requests = NULL;

    // TODO: Further error handling
    if (result.cycles == 0 && result.misses == 0 && result.hits == 0 && result.primitiveGateCount == 0) {
        fprintf(stderr, "Result does not contain valid data.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b
    fprintf(stdout,
            "--------------------------------------------------\n"
            "\x1b[1m\t\tSimulation Results\x1b[0m\n"
            "--------------------------------------------------\n"
            "\tCycles:\t%zu\n\tMisses:\t%zu\n\tHits:\t%zu\n\tPrimitive gate count:\t%zu\n",
            result.cycles, result.misses, result.hits, result.primitiveGateCount);

    fprintf(stdout, "End of Simulation\n");

    return EXIT_SUCCESS;
}
