#pragma once

#include "stddef.h"
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
    int usingCache;
    int callExtended;
};

void print_usage(const char* progname);

int parse_arguments(int argc, char** argv, struct Configuration* config);
