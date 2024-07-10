#include "Cache.h"
#include "Request.h"

#include <systemc>

// optimsiation: instruction buffer
SC_MODULE(InstructionCache) {
  private:
    CACHE rawCache;
    std::vector<Request> instructions;

    SC_CTOR(InstructionCache);

    void decode() {
        // dummy -> we dont actually do anything with the value
        // int dummy = readFromRawCache;
        auto pc = pcBus.read();
        if (pc >= instructions.size()) {
            sc_core::sc_stop();
        }
        instructionBus.write(instructions[pc]);
        instrReadyBus.write(true);
    }

    void fetch() {
        instrReadyBus.write(false);
        // pass on to cache
    }

  public:
    // -> CPU
    sc_core::sc_out<Request> instructionBus;
    sc_core::sc_out<bool> instrReadyBus;
    // <- CPU
    sc_core::sc_in<bool> validInstrRequestBus;
    sc_core::sc_in<std::uint32_t> pcBus;
    // -> RAM (same as cache)
    // <- RAM (Dummys, we do not write)

    SC_METHOD(decode);
    sensitive << cache.ready;
    dont_initialize();

    SC_METHOD(fetch);
    sensitive << validInstrRequestBus;
    dont_initialize();
}