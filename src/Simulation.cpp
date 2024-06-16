#include "Simulation.h"

#include "CPU.h"
#include "ReadOnlySpan.h"
#include "cache.hpp"

void doSimulation() {
    const Request testRequestArray[4] = {Request{0, 10, 1}, Request{0, 0, 0}, Request{0, -1u, 1}, Request{0, 0, 0}};
    CPU cpu{"CPU", 3, ReadOnlySpan<Request>{testRequestArray, 4}};
    CACHE cache{"Cache", 0, 10, 100, 1, 4};
}