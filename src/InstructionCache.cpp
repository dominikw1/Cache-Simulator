#include "InstructionCache.h"

using namespace sc_core;
InstructionCache::InstructionCache(sc_module_name name) : sc_module{name} {
    // rawCache{"instrCache", 100, 10, 10, {"Cache", 10, 64, 10, std::make_unique<RandomPolicy<std::uint32_t>>(10)}} {

    SC_METHOD(decode);
    sensitive << cache.ready;
    dont_initialize();
    SC_METHOD(fetch);
    sensitive << validInstrRequestBus;
    dont_initialize();
}

void InstructionCache::fetch() {
    instrReadyBus.write(false);
    // pass on to cache
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