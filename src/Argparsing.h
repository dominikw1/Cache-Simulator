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

static const char* usage_msg =
    "usage: %s <filename> [-c c/--cycles c] [--lcycles] [--directmapped] [--fullassociative] "
    "[--cacheline-size s] [--cachelines n] [--cache-latency l] [--memorylatency m] "
    "[--lru] [--fifo] [--random] [--use-cache=<Y,n>] [--tf=<filename>] [--extended] [-h/--help]\n"
    "   -c c / --cycles c       Set the number of cycles to be simulated to c. Allows inputs in range [0,2^16-1]\n"
    "   --lcycles               Allow input of cycles of up to 2^32-1\n"
    "   --directmapped          Simulate a direct-mapped cache\n"
    "   --fullassociative       Simulate a fully associative cache\n"
    "   --cacheline-size s      Set the cache line size to s bytes\n"
    "   --cachelines n          Set the number of cachelines to n\n"
    "   --cache-latency l       Set the cache latency to l cycles\n"
    "   --memory-latency m      Set the memory latency to m cycles\n"
    "   --lru                   Use LRU as cache-replacement policy\n"
    "   --fifo                  Use FIFO as cache-replacement policy\n"
    "   --random                Use random cache-replacement policy\n"
    "   --use-cache=<Y,n>       Simulates a system with cache or no cache\n"
    "   --extended              Call extended run_simulation-method\n"
    "   --tf=<filename>         File name for a trace file containing all signals. If not set, no "
    "trace file will be created\n"
    "   -h / --help             Show help message and exit\n";

void print_usage(const char* progname);

int parse_arguments(int argc, char** argv, struct Configuration* config);
