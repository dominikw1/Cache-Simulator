#pragma once

#include "stddef.h"
#include "Request.h"
#include "Simulation/Policy/Policy.h"

/**
 * This structure contains all parameters needed for the simulation including
 * the additional parameters 'policy' and 'callExtended' used for an
 * extension of the simulation method.
 */
struct Configuration {
    unsigned int cycles;
    int directMapped;
    unsigned int cacheLines;
    unsigned int cacheLineSize;
    unsigned int cacheLatency;
    unsigned int memoryLatency;
    size_t numRequests;
    struct Request* requests;
    const char* tracefile;
    enum CacheReplacementPolicy policy;
    int callExtended;
};

void print_usage(const char* progname);

/**
 * This function processes command-line arguments, sets default configuration values,
 * and validates the input. It returns a configuration structure containing
 * the parsed values.
 */
struct Configuration parse_arguments(int argc, char** argv);
