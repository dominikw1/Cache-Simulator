#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <getopt.h>
#include <sys/stat.h>
#include <errno.h>

#include "Request.h"
#include "Result.h"

#define DIRECTMAPPED 128
#define FULLASSOCIATIVE 129
#define CACHELINE_SIZE 130
#define CACHELINES 131
#define CACHE_LATENCY 132
#define MEMORY_LATENCY 133
#define TRACEFILE 134

extern struct Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
                                    unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                                    struct Request requests[], const char* tracefile);

// Taken and adapted from GRA Week 3 "Nutzereingaben" and "File IO"
const char* usage_msg = "usage: %s <filename> [-c/--cycles c] [--directmapped] [--fullassociative] "
                        "[--cacheline-size s] [--cachelines n] [--cache-latency l] "
                        "[--memorylatency m] [--tf=<filename>] [-h/--help]\n"
                        "   -c c / --cycles c       Set the number of cycles to be simulated to c\n"
                        "   --directmapped          Simulate a direct-mapped cache\n"
                        "   --fullassociative       Simulate a fully associative cache\n"
                        "   --cacheline-size s      Set the cache line size to s bytes\n"
                        "   --cachelines n          Set the number of cachelines to n\n"
                        "   --cache-latency l       Set the cache latency to l cycles\n"
                        "   --memory-latency m      Set the memory latency to m cycles\n"
                        "   --tf=<filename>         File name for a trace file containing all signals. If not set, no "
                        "trace file will be created\n"
                        "   -h / --help             Show help message and exit\n";

const char* help_msg = "Positional arguments:\n"
                       "   <filename>   The name of the file to be processed\n"
                       "\n"
                       "Optional arguments:\n"
                       "   -c c / --cycles c       The number of cycles used for the simulation (default: c = 0)\n"
                       "   --directmapped          Simulates a direct-mapped cache\n"
                       "   --fullassociative       Simulates a fully associative cache (Set as default)\n"
                       "   --cacheline-size s      The size of a cache line in bytes (default: 32)\n"
                       "   --cachelines n          The number of cache lines (default: 2048)\n"
                       "   --cache-latency l       The cache latency in cycles (default: 1)\n"
                       "   --memory-latency m      The memory latency in cycles (default: 100)\n"
                       "   --tf=<filename>         The name for a trace file containing all signals. If not set, no "
                       "trace file will be created\n"
                       "   -h / --help             Show this help message and exit\n";

void print_usage(const char* progname) { fprintf(stderr, usage_msg, progname, progname, progname); }

void print_help(const char* progname) { print_usage(progname); fprintf(stderr, "\n%s", help_msg); }

// Taken from: https://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
int isPowerOfTwo(unsigned long n) { return n && !(n & (n - 1)); }

FILE* check_file (const char* progname, const char* filename) {
    if (filename == NULL) {
        perror("Filename does not exist");
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    // Lines 78-98 taken and adapted from GRA Week 3 "File IO" files.c
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    if (!S_ISREG(file_info.st_mode)) {
        fprintf(stderr, "%s is not a regular file\n", filename);
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(file_info.st_mode)) {
        fprintf(stderr, "Filename should not be a directory.\n");
        fclose(file);
        print_usage(progname);
        exit(EXIT_FAILURE);
    }

    return file;
}

struct Request* extract_file_data (const char* progname, FILE* file, struct Request* requests, size_t* numRequests) {
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
                fprintf(stderr, "Wrong file format! An Error occurred while reading the file.\n");
                fclose(file);
                print_usage(progname);
                exit(EXIT_FAILURE);
            }
            if (read_line < 2) { // Address was not read from file
                fprintf(stderr, "Wrong file format! No address given.\n");
                fclose(file);
                print_usage(progname);
                exit(EXIT_FAILURE);
            }
        }

        if (we == 'W' && read_line < 3) {   // Write should have data written in the file
            fprintf(stderr, "Wrong file format! No data saved.\n");
            fclose(file);
            print_usage(progname);
            exit(EXIT_FAILURE);
        }
        if (we == 'R' && read_line == 3) {  // Read should not have data written in the file
            fprintf(stderr, "Wrong file format! When reading from a file, data should be empty.\n");
            fclose(file);
            print_usage(progname);
            exit(EXIT_FAILURE);
        }

        requests[*numRequests].addr = addr;
        requests[*numRequests].data = data;
        if (we == 'R') {
            requests[*numRequests].we = 0;
        } else if (we == 'W') {
            requests[*numRequests].we = 1;
        } else {
            fprintf(stderr, "Not a valid operation.\n");
            fclose(file);
            print_usage(progname);
            exit(EXIT_FAILURE);
        }

        (*numRequests)++;

        if (ferror(file)) {
            printf("Error reading file.\n");
            fclose(file);
            print_usage(progname);
            exit(EXIT_FAILURE);
        }

    } while (!feof(file));

    fclose(file);
    return requests;
}

unsigned long check_user_input(char* endptr, char* message, const char* progname, char* option) {
    endptr = NULL;
    long n = strtol(optarg, &endptr, 10);
    // TODO: Further error handling: Negative Inputs
    if (*endptr != '\0' || n <= 0 || errno != 0) {
        if (errno == 0 && n <= 0) {
            fprintf(stderr, "Invalid input: %s\n", message);
        } else if (n > INT32_MAX) {
            fprintf(stderr, "%ld is too big to be converted to an int.\n", n);
        } else {
            char* error_message = strerror(errno);
            fprintf(stderr, "Error parsing number for option %s%s", option, error_message);
        }
        print_usage(progname);
        exit(EXIT_FAILURE);
    }
    return n;
}


int main(int argc, char** argv) {

    const char* progname = argv[0];

    if (argc == 1) {
        fprintf(stderr, "Positional argument missing!\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // TODO: Set useful default values
    int cycles = 0;
    int directMapped = 0; // 0 => fullassociative, x => directmapped
    unsigned int cacheLines = 2048;
    unsigned int cacheLineSize = 32;
    unsigned int cacheLatency = 1;    // in cycles
    unsigned int memoryLatency = 100;
    const char* tracefile = NULL;
    // TODO: Implement option Cache Replacement Policy

    // Extract file data
    FILE* file = check_file(progname, argv[1]);
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        fclose(file);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    size_t numRequests = 0;
    struct Request *requests = (struct Request *) malloc(sizeof(struct Request)*file_info.st_size);
    if (requests == NULL) {
        perror("Error allocating memory buffer for file.");
        fclose(file);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    requests = extract_file_data(progname, file, requests, &numRequests);

    // PARSING
    int opt;
    int option_index;
    char* error_message;
    char* endptr = NULL;
    const char* optstring = "c:h";
    static struct option long_options[] = {
        {"cycles", required_argument, 0, 'c'},
        {"directmapped", no_argument, 0, DIRECTMAPPED},
        {"fullassociative", no_argument, 0, FULLASSOCIATIVE},
        {"cacheline-size", required_argument, 0, CACHELINE_SIZE},
        {"cachelines", required_argument, 0, CACHELINES},
        {"cache-latency", required_argument, 0, CACHE_LATENCY},
        {"memory-latency", required_argument, 0, MEMORY_LATENCY},
        {"tf=", required_argument, 0, TRACEFILE},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
        errno = 0;

        switch (opt) {
        case 'c':
            error_message = "Cycles cannot be smaller than 1.";
            unsigned long c = check_user_input(endptr, error_message, progname, "-c/--cycles");
            cycles = (int) c;
            break;

        case 'h':
            print_help(progname);
            return EXIT_SUCCESS;

        case DIRECTMAPPED:
            directMapped = 1;
            break;

        case FULLASSOCIATIVE:
            // Default value is set to fullassociative
            break;

        case CACHELINE_SIZE:
            error_message = "Cacheline size should be at least 1.";
            unsigned long s = check_user_input(endptr, error_message, progname, "--cacheline-size");

            if (!isPowerOfTwo(s)) {
                fprintf(stderr, "Invalid Input: Cacheline size should be a power of two!\n");
                print_usage(progname);
                return EXIT_FAILURE;
            }

            cacheLineSize = (int) s;
            break;

        case CACHELINES:
            error_message = "Number of cache-lines must be at least 1.";
            unsigned long n = check_user_input(endptr, error_message, progname, "--cachelines");
            cacheLines = (int) n;
            break;

        case CACHE_LATENCY:
            error_message = "Cache-latency cannot be zero or negativ.";
            unsigned long l = check_user_input(endptr, error_message, progname, "--cache-latency");
            cacheLatency = (int) l;
            break;

        case MEMORY_LATENCY:
            error_message = "Memory-latency cannot be zero or negativ.";
            unsigned long m = check_user_input(endptr, error_message, progname, "--memory-latency");
            memoryLatency = (int) m;
            break;

        case TRACEFILE:
            FILE* t_file = check_file(progname, optarg);

            char* tracefile_copy = NULL;
            strncpy(tracefile_copy, optarg, strlen(optarg));
            tracefile = tracefile_copy;

            fclose(t_file);
            break;

        default:
            print_usage(progname);
            return EXIT_FAILURE;
        }
    }

    if (memoryLatency < cacheLatency) {
        // TODO: Wenn Memory < Cache latency => Warning
    }

    // TODO: Cachelines und CachlineSize sind kleiner als 32 Bit

    run_simulation(cycles, directMapped, cacheLines, cacheLineSize, cacheLatency, memoryLatency,
                   numRequests, requests, tracefile);
    free(requests);

    return EXIT_SUCCESS;
}