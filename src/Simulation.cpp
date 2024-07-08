#include "Simulation.h"
#include "CPU.h"
#include "ReadOnlySpan.h"
#include "Request.h"
#include "Result.h"
// #include "Cache.h"

#include <systemc>
struct Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
                             unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                             struct Request requests[], const char* tracefile) {
    std::cout << "Starting Simulation...\n";
    CPU cpu{"CPU"};
    //   CACHE cache{"Cache", directMapped, cacheLines, cacheLineSize, cacheLatency, memoryLatency};
    return Result{};
}

using namespace sc_core;
int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR" << std::endl;
    return 1;
}