#include "Simulation.h"

#include "CPU.h"
#include "ReadOnlySpan.h"
#include "Request.h"
#include "Result.h"
#include "cache.hpp"

Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize, unsigned int cacheLatency,
                      unsigned int memoryLatency, size_t numRequests, struct Request requests[], const char* tracefile) {

    CPU cpu{"CPU", 3, ReadOnlySpan<Request>{requests, 4}};
    CACHE cache{"Cache", directMapped, cacheLines, cacheLineSize, cacheLatency, memoryLatency};
    return Result{};
}

int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR" << std::endl;
    return 1;
}