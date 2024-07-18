#pragma once

#include "Policy/Policy.h"
#include "Request.h"
#include "Result.h"
#include "stddef.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
struct Result run_simulation_extended(uint32_t cycles, int directMapped, unsigned int cacheLines,
                                      unsigned int cacheLineSize, unsigned int cacheLatency, unsigned int memoryLatency,
                                      size_t numRequests, struct Request requests[], const char* tracefile,
                                      CacheReplacementPolicy policy, int usingCache);

struct Result run_simulation(int cycles, int directMapped, unsigned cacheLines, unsigned cacheLineSize,
                             unsigned cacheLatency, unsigned memoryLatency, size_t numRequests,
                             struct Request requests[], const char* tracefile);
;
#ifdef __cplusplus
}
#endif