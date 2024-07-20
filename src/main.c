#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "Argparsing.h"
#include "Request.h"
#include "Result.h"
#include "Simulation/Simulation.h"

int main(int argc, char** argv) {

    // Parse command line arguments
    struct Configuration configuration;
    parse_arguments(argc, argv, &configuration);

    // Call run_simulation method depending on user input for extended method
    struct Result result;
    if (configuration.callExtended) {
        result = run_simulation_extended(configuration.cycles, configuration.directMapped, configuration.cacheLines,
                                         configuration.cacheLineSize, configuration.cacheLatency,
                                         configuration.memoryLatency, configuration.numRequests, configuration.requests,
                                         configuration.tracefile, configuration.policy, configuration.usingCache);
    } else {
        result = run_simulation((int)configuration.cycles, configuration.directMapped, configuration.cacheLines,
                                configuration.cacheLineSize, configuration.cacheLatency, configuration.memoryLatency,
                                configuration.numRequests, configuration.requests, configuration.tracefile);
    }
    free(configuration.requests);
    configuration.requests = NULL;

    // Check for invalid results
    if (result.cycles == 0 && result.misses == 0 && result.hits == 0 && result.primitiveGateCount == 0) {
        fprintf(stderr, "Result does not contain valid data.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Used ANSI Escape Sequence from https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b
    fprintf(stdout,
            "\x1b[1m--------------------------------------------------\x1b[0m\n"
            "\x1b[1m\t\tSimulation Results\x1b[0m\n"
            "\x1b[1m--------------------------------------------------\x1b[0m\n"
            "\tCycles:\t%zu\n"
            "\tMisses:\t\x1b[31m%zu\t\t\x1b[0m\n"
            "\tHits:\t\x1b[32m%zu\t\t\x1b[0m\n"
            "\tPrimitive gate count:\t%zu\x1b[0m\n"
            "\x1b[1m--------------------------------------------------\x1b[0m\n"
            "\x1b[0m",
            result.cycles, result.misses, result.hits, result.primitiveGateCount);

    return EXIT_SUCCESS;
}
