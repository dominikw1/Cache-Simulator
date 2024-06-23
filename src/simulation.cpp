#include "simulation.hpp"

#include "result.h"
#include "request.h"

struct Result run_simulation(int cycles, int directMapped, 
    unsigned cacheLines, unsigned cacheLineSize, unsigned cacheLatency, unsigned memoryLatency, 
    size_t numRequests , struct Request requests[], const char* tracefile) {

    struct Result result;
    result.cycles = 0;
    return result;

}

int sc_main(int argc, char* argv[]) {
    std::cout << "ERROR" << std::endl;
    return 1;
}