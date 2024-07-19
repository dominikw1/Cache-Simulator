#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "Simulation/Policy/Policy.h"
#include "Request.h"
#include "Result.h"
#include "Simulation/Simulation.h"
#include "Argparsing.h"


int main(int argc, char** argv) {
    struct Configuration configuration;
    int exitFailure = parse_arguments(argc, argv, &configuration);    // Parse command line arguments

    if(exitFailure) {
        return EXIT_FAILURE;
    }

    struct Result result;
    if (configuration.callExtended) {
        result = run_simulation_extended(configuration.cycles, configuration.directMapped, configuration.cacheLines,
                                         configuration.cacheLineSize, configuration.cacheLatency,
                                         configuration.memoryLatency, configuration.numRequests, configuration.requests,
                                         configuration.tracefile, configuration.policy, configuration.usingCache);
    } else {
        result = run_simulation(configuration.cycles, configuration.directMapped, configuration.cacheLines,
                                configuration.cacheLineSize, configuration.cacheLatency, configuration.memoryLatency,
                                configuration.numRequests, configuration.requests, configuration.tracefile);
    }

    free(configuration.requests);
    configuration.requests = NULL;

    // TODO: Further error handling
    if (result.cycles == 0 && result.misses == 0 && result.hits == 0 && result.primitiveGateCount == 0) {
        fprintf(stderr, "Result does not contain valid data.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    // Print Simulation results
    // https://gist.github.com/ConnerWill/d4b6c776b509add763e17f9f113fd25b
    fprintf(stdout,
            "--------------------------------------------------\n"
            "\x1b[1m\t\tSimulation Results\x1b[0m\n"
            "--------------------------------------------------\n"
            "\tCycles:\t%zu\n\tMisses:\t%zu\n\tHits:\t%zu\n\tPrimitive gate count:\t%zu\n",
            result.cycles, result.misses, result.hits, result.primitiveGateCount);

    return EXIT_SUCCESS;
}
