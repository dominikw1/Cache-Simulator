#pragma once

#include "stddef.h"
#include "Request.h"
#include "Simulation/Policy/Policy.h"


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

struct Configuration parse_arguments(int argc, char** argv);
