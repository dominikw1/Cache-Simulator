#pragma once
#include "OrGate.h"
#include "RingQueue.h"
#include <mutex>
#include <systemc>

struct WriteBufferEntry {
    std::uint32_t address;
    std::uint32_t data;
};

template <std::uint8_t SIZE> SC_MODULE(WriteBuffer) {
  public:
    sc_core::sc_in<bool> clock;

    // Buffer -> Cache
    sc_core::sc_out<bool> SC_NAMED(ready);
    sc_core::sc_out<sc_dt::sc_bv<128>> SC_NAMED(cacheDataOutBus);

    // Cache -> Buffer
    sc_core::sc_in<std::uint32_t> SC_NAMED(cacheAddrBus);
    sc_core::sc_in<std::uint32_t> SC_NAMED(cacheDataInBus);
    sc_core::sc_in<bool> SC_NAMED(cacheWeBus);
    sc_core::sc_in<bool> SC_NAMED(cacheValidRequest);

    // Buffer -> RAM
    sc_core::sc_out<std::uint32_t> SC_NAMED(memoryAddrBus);
    sc_core::sc_out<std::uint32_t> SC_NAMED(memoryDataOutBus);
    sc_core::sc_out<bool> SC_NAMED(memoryWeBus);
    sc_core::sc_out<bool> SC_NAMED(memoryValidRequestBus);

    // RAM -> Buffer
    sc_core::sc_in<sc_dt::sc_bv<128>> SC_NAMED(memoryDataInBus);
    sc_core::sc_in<bool> SC_NAMED(memoryReadyBus);

    // Internal Signals
    sc_core::sc_signal<bool> writerThreadValidMemoryRequest;
    sc_core::sc_signal<bool> readerThreadValidMemoryRequest;
    OrGate SC_NAMED(validMemoryRequestOr);

    WriteBuffer(sc_core::sc_module_name name, std::uint32_t readsPerCacheline)
        : sc_module{name}, readsPerCacheline{readsPerCacheline} {
        using namespace sc_core;

        validMemoryRequestOr.a.bind(writerThreadValidMemoryRequest);
        validMemoryRequestOr.b.bind(readerThreadValidMemoryRequest);
        validMemoryRequestOr.out.bind(memoryValidRequestBus);

        SC_THREAD(handleIncomingRequests);
        //    sensitive << clock.pos();
        SC_THREAD(doWrites);
        //  sensitive << clock.pos();
    }

  private:
    SC_CTOR(WriteBuffer);

    RingQueue<WriteBufferEntry> buffer{SIZE};
    std::mutex currWritingMarkerLock;
    bool currWritingMarker = false;

    void setCurrRequest(bool newValue) {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        currWritingMarker = newValue;
    }

    bool currentlyWriting() {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        return currWritingMarker;
    }

    void relinquishWriting() {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        currWritingMarker = false;
    }

    bool tryAcquireWriting() {
        std::lock_guard<std::mutex> g{currWritingMarkerLock};
        if (!currWritingMarker) {
            currWritingMarker = true;
            return true;
        } else {
            return false;
        }
    }

    void writeToRAM() {
        std::cout << "Write Buffer: Popping off next write " << std::endl;
        assert(buffer.getSize() > 0);
        WriteBufferEntry next = buffer.pop();
        memoryAddrBus.write(next.address);
        memoryDataOutBus.write(next.data);
        memoryWeBus.write(true);
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG
        writerThreadValidMemoryRequest.write(true);
        std::cout << "WriteBuffer: " << "Waiting for RAM write " << std::endl;
        wait(clock.posedge_event());
        do {
            wait(clock.posedge_event());
        } while ((!memoryReadyBus.read()));
        std::cout << "WriteBuffer: " << "Done waiting for RAM write " << std::endl;
        writerThreadValidMemoryRequest.write(false);
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG
        wait(clock.posedge_event());
    }

    const std::uint32_t readsPerCacheline;

    void passReadAlong() {
        memoryAddrBus.write(cacheAddrBus.read());
        memoryWeBus.write(false);
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG
        readerThreadValidMemoryRequest.write(true);

        std::cout << "WriteBuffer: " << "Now waiting for RAM" << std::endl;
        do {
            wait(clock.posedge_event());
        } while (!memoryReadyBus.read());
        readerThreadValidMemoryRequest.write(false);
        wait(clock.posedge_event());
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG

        std::cout << "WriteBuffer: " << " Done waiting for RAM" << std::endl;

        ready.write(true);
        // dont need to wait before first one because we can only get here if RAM tells us it is ready
        for (int i = 0; i < readsPerCacheline; ++i) {
            cacheDataOutBus.write(memoryDataInBus.read());
            wait(clock.posedge_event());
        }

        ready.write(false);
        wait(clock.posedge_event()); // DEBUG
        wait(clock.posedge_event());
    }



    void handleIncomingRequests() {
        bool holdingWriteLock = false;
        while (true) {
            wait(clock.posedge_event());
            if (cacheValidRequest.read()) {
                //    std::cout << "WriteBuffer: " << "WB: found valid instr request" << std::endl;
                //  std::cout << "WriteBuffer: " << "State: " << cacheWeBus.read() << " " << buffer.getSize() << " "
                //          << currentlyWriting() << std::endl;

                if (!cacheWeBus.read() && buffer.getSize() == 0 && !currentlyWriting()) {
                    // can only handle if all writes are through for sequential consistency reasons
                    std::cout << "WriteBuffer: " << "Passing along read " << std::endl;
                    passReadAlong();
                    wait(clock.posedge_event());
                    ready.write(false);
                } else {
                    if (cacheWeBus.read() && buffer.getSize() != SIZE) {
                        // can only handle if there is space in buffer
                        std::cout << "Placing new write buffer request in buffer " << std::endl;
                        buffer.push(WriteBufferEntry{cacheAddrBus.read(), cacheDataInBus.read()});
                        std::cout << "New size: " << buffer.getSize() << std::endl;
                        ready.write(true);
                        wait(clock.posedge_event());
                        ready.write(false);
                    }
                }
            }
            wait(clock.posedge_event());
        }
    }

    void doWrites() {
        while (true) {
            wait(clock.posedge_event());
            //   std::cout << "WriteBuffer: " << "Entering write loop " << std::endl;
            if (cacheValidRequest.read() && !cacheWeBus.read() && buffer.getSize() == 0) {
                continue;
            }
            //  std::cout << "WriteBuffer: " << "Trying to write" << std::endl;
            if (buffer.getSize() > 0) {
                std::cout << "WriteBuffer: " << "Entering actual write" << std::endl;
                if (tryAcquireWriting()) {
                    writeToRAM();
                    relinquishWriting();
                } else {
                    std::cout << "Failed to acquire lock" << std::endl;
                }
                std::cout << "WriteBuffer: " << "Ending actual write" << std::endl;
                std::cout << "WriteBuffer: ending cycle at size " << buffer.getSize() << std::endl;
            }
        }
    }
};
