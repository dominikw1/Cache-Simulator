#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>
#include <errno.h>
#include <unistd.h>


// Taken and adapted from GRA Week 3 "Nutzereingaben"
const char* usage_msg =    // TODO: finalise usage message
    "usage: %s <filename> [-c/--cycles c] [--directmapped] [--fullassociative] [--cacheline-size s] [--cachelines n] [--cache-latency l] [--memorylatency m] [--tf=<filename>] [-h/--help]\n"
    "   -c c / --cycles c       Set the number of cycles to c\n" 
    "   --directmapped          ...\n"
    "   --fullassociative       ...\n"
    "   --cacheline-size s      ...\n"
    "   --cachelines n          ...\n"
    "   --cache-latency l       ...\n"
    "   --memory-latency m      ...\n"
    "   --tf=<filename>         ...\n"
    "   -h / --help             Show help message and exit\n";

const char* help_msg =  // TODO: finalise help message
    "Positional arguments:\n"
    "   <filename>   The name of the file to be processed.\n"
    "\n"
    "Optional arguments:\n"
    "   -c c / --cycles c       The length of a cycle (default: c = 0)\n" 
    "   --directmapped          ...\n"
    "   --fullassociative       ...\n"
    "   --cacheline-size s      ...\n"
    "   --cachelines n          ...\n"
    "   --cache-latency l       ...\n"
    "   --memory-latency m      ...\n"
    "   --tf=<filename>         ...\n"
    "   -h / --help             Show help message (this text) and exit\n";

void print_usage(const char* progname) {
    fprintf(stderr, usage_msg, progname, progname, progname);
}

void print_help(const char* progname) {
    print_usage(progname);
    fprintf(stderr, "\n%s", help_msg);
}

// TODO: Call to run_simulation(cacheSize, directmapped, requests)

int main(int argc, char **argv) {

    const char* progname = argv[0];

    if (argc == 1) {
        print_usage(progname);
        return EXIT_FAILURE;
    }

    // TODO: Extract file data and save to requests
    const char* filename = argv[1];
    // TODO: Error handling
    

    // TODO: Set useful default values
    int cycles = 0;
    int directMapped = 0; // 0 => fullassociative, x => directmapped
    unsigned cacheLines = 2048;
    unsigned cacheLineSize = 32;
    unsigned cacheLatency = 1;  // in cycles
    unsigned memoryLatency = 100;
    const char* tracefile = NULL;

    // PARSING
    int opt;
    char *endptr;
    int option_index;
    const char* optstring = "c:h";
    static struct option long_options[] = { // TODO: Change nums to "smarter" values
        {"cycles", required_argument, 0, 'c'},
        {"directmapped", no_argument, 0, 128},
        {"fullassociative", no_argument, 0, 129},
        {"cacheline-size", required_argument, 0, 130},
        {"cachelines", required_argument, 0, 131},
        {"cache-latency", required_argument, 0, 132},
        {"memory-latency", required_argument, 0, 133},
        {"tf=", required_argument, 0, 134},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
   };

    while ((opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1) {
        switch(opt) {
            case 'c':
                // TODO: Error handling
                // strtol() => returns long int => returns 0 if not converted
                cycles = strtol(optarg, &endptr, 10);   // Cycles in decimal
            case 'h':
                print_help(progname);
                return EXIT_SUCESS;
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
                cacheLineSize = strtoul(optarg, &endptr, 10);   // cacheline-size in decimal
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
                memoryLatency = strtoul(optarg, &endptr, 10);   // memory-latency
                break;
            case 134:
                // TODO: Create Tracefile
                tracefile = "";
                // TODO: Error handling
                break;

            case '?':
                print_usage(argv[0]);
                return EXIT_FAILURE;
            default: 
                // TODO: Default Error Message
                return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}