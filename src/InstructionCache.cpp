#include "InstructionCache.h"

using namespace sc_core;

InstructionCache::InstructionCache(sc_module_name name, unsigned int cacheLines, unsigned int cacheLineSize,
                                   std::vector<Request> instructions)
        : sc_module{name}, cacheLineNum{cacheLines}, cacheLineSize{cacheLineSize}, instructions{instructions},
          memoryDataInBusses{cacheLineSize}, memoryDataOutBusses{cacheLineSize} {

    SC_METHOD(decode);
    sensitive << cache.ready;
    dont_initialize();

    SC_METHOD(fetch);
    sensitive << validInstrRequestBus;
    dont_initialize();
}

void InstructionCache::fetch() {
    instrReadyBus.write(false);
    cache.memoryAddrBus.write(pcBus.read());
    cache.memoryWeBus.write(false);
    cache.memoryValidRequestBus.write(true);
}

void InstructionCache::decode() {
    // dummy -> we dont actually do anything with the value
    // int dummy = readFromRawCache;
    auto pc = pcBus.read();
    if (pc >= instructions.size()) {
        sc_core::sc_stop(); // cant do this -> last instruction has to go through...
    }
    instructionBus.write(instructions[pc]);
    instrReadyBus.write(true);
}