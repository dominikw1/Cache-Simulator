#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "Request.h"
#include "Result.h"


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

void print_help(const char* progname) {
    print_usage(progname);
    fprintf(stderr, "\n%s", help_msg);
}

int main(int argc, char** argv) {

    const char* progname = argv[0];

    if (argc == 1) {
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // TODO: Set useful default values
    int cycles = 0;
    int directMapped = 0; // 0 => fullassociative, x => directmapped
    unsigned int cacheLines = 2048;
    unsigned int cacheLineSize = 32;
    unsigned int cacheLatency = 1;    // in cycles
    unsigned int memoryLatency = 100; // TODO: Wenn Memory < Cache latency => Warning
    const char* tracefile = NULL;

    // TODO: Extract file data and save to requests
    const char* filename = argv[1];
    if (filename == NULL) {
        // TODO: Error Message
        perror("Filename does not exist.");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file.\n");
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // Lines 97-118 taken and adapted from GRA Week 3 "File IO" files.c l.17-34
    struct stat file_info;
    if (fstat(fileno(file), &file_info) != 0) {
        perror("Error determining file size");
        fclose(file);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    if (!S_ISREG(file_info.st_mode)) {
        fprintf(stderr, "%s is not a regular file\n", filename);
        fclose(file);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    size_t numRequests = 0;
    struct Request *requests = (struct Request *) malloc(file_info.st_size);
    if (requests == NULL) {
        perror("Error allocating memory buffer for file.");
        fclose(file);
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // TODO: Check for invalid file format and save file content to requests
    // Inspired by: https://github.com/portfoliocourses/c-example-code/blob/main/csv_to_struct_array.c
    int read_line = 0;

    do {
        char we;
        uint32_t addr;
        uint32_t data;

        read_line = fscanf(file, "%c,%i,%i\n", &we, &addr, &data);
        // TODO: Handle wrong file format
        /*if (read < 3 && !feof(file)) {
            if (we == 'R' && data != NULL) {
                // Fehler: Wert in dritter Spalte ist bei R nicht leer

            }
            // TODO: Wrong file format
            return EXIT_FAILURE;
        }
        if (read == 2 && !feof(file)) {
            if (we == 'W' && data == NULL) {
                // Fehler: Schreibzugriff aber kein Datum gegeben
            }
        }
        // Fehler: Wert in dritter Spalte ist bei R nicht leer
        */

        requests[numRequests].addr = addr;
        requests[numRequests].data = data;
        if (we == 'R') {
            requests[numRequests].we = 0;
        } else if (we == 'W') {
            requests[numRequests].we = 1;
        } // Fehlerbehandlung: Nicht R/W
        numRequests++;

        if (ferror(file)) {
            printf("Error reading file.\n");
            print_usage(progname);
            return EXIT_FAILURE;
        }

    } while (!feof(file)); // TODO: feof nachschlagen!!

    fclose(file);

    // TODO: Error handling

    // PARSING
    int opt;
    char* endptr;
    int option_index;
    const char* optstring = "c:h";
    static struct option long_options[] = {// TODO: Change nums to "smarter" values => MICROS verwenden für Zahlen
                                           {"cycles", required_argument, 0, 'c'},
                                           {"directmapped", no_argument, 0, 128},
                                           {"fullassociative", no_argument, 0, 129},
                                           {"cacheline-size", required_argument, 0, 130},
                                           {"cachelines", required_argument, 0, 131},
                                           {"cache-latency", required_argument, 0, 132},
                                           {"memory-latency", required_argument, 0, 133},
                                           {"tf=", required_argument, 0, 134},
                                           {"help", no_argument, 0, 'h'},
                                           {0, 0, 0, 0}};

    while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
        switch (opt) {
        case 'c':
            // TODO: Error handling
            // strtol() => returns long int => returns 0 if not converted
            cycles = strtol(optarg, &endptr, 10); // Cycles in decimal
            break;
        case 'h':
            print_help(progname);
            return EXIT_SUCCESS;
            break;
        case 128:
            // TODO: Error handling w errno :o
            directMapped = 1;
            break;
        case 129:
            // Default value is set to fullassociative
            break;
        case 130:
            // TODO: Error handling
            cacheLineSize = strtoul(optarg, &endptr, 10); // cacheline-size in decimal
            break;
        case 131:
            // TODO: Error handling
            cacheLines = strtoul(optarg, &endptr, 10);
            break;
        case 132:
            // TODO: Error handling
            cacheLatency = strtoul(optarg, &endptr, 10);
            break;
        case 133:
            // TODO: Error handling
            memoryLatency = strtoul(optarg, &endptr, 10); // memory-latency
            break;
        case 134:
            // TODO: Create Tracefile
            tracefile = "";
            // TODO: Error handling
            break;

        case '?':
            print_usage(progname);
            return EXIT_FAILURE;
        default:
            // TODO: Default Error Message
            return EXIT_FAILURE;
        }
    }

    struct Result result = run_simulation(cycles, directMapped, cacheLines, cacheLineSize, cacheLatency, memoryLatency,
                                          numRequests, requests, tracefile);
    free(requests);

    return EXIT_SUCCESS;
}