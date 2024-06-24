#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include "systemc.h"
#include <systemc>

#include "Result.h"
#include <stddef.h>

struct Request;
struct Result;

extern "C" Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
                                 unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                                 struct Request requests[], const char* tracefile);
#endif