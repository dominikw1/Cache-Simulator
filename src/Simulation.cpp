#include "Simulation.h"
#include "CPU.h"
#include "Cache.h"
#include "InstructionCache.h"
#include "Request.h"
#include "Result.h"
#include "Policy.h"
#include "InstructionCache.h"

#include <systemc>
struct Result run_simulation(int cycles, int directMapped, unsigned int cacheLines, unsigned int cacheLineSize,
                             unsigned int cacheLatency, unsigned int memoryLatency, size_t numRequests,
                             struct Request requests[], const char* tracefile, int policy, int usingCache) {
    std::cout << "Starting Simulation...\n";
    CPU cpu{"CPU"};
    //CACHE<MappingType::DIRECT_MAPPED> cache{"Cache", cacheLines, cacheLineSize};
    
    sc_core::sc_start();
    return Result{};
}

using namespace sc_core;
int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR: call to sc_main method" << std::endl;
    return 1;
}