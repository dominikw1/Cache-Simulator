#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <systemc>
#include "systemc.h"

using namespace sc_core;

extern "C" struct Result run_simulation(
    int cycles,
    int directMapped,
    unsigned cacheLines,
    unsigned cacheLineSize,
    unsigned cacheLatency,
    unsigned memoryLatency,
    size_t numRequests,
    struct Request requests[],
    const char* tracefile
    );


#endif