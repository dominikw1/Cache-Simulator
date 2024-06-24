#pragma once

#include "Request.h"
#include "Result.h"
#include "stddef.h"
#ifdef __cplusplus
extern "C" {
#endif
struct Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
                             unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                             struct Request requests[], const char* tracefile);
#ifdef __cplusplus
}
#endif