#pragma once
#include "AndGate.h"
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
    //  sc_core::sc_signal<bool> SC_NAMED(writerThreadValidMemoryRequest);
    //    sc_core::sc_signal<bool> SC_NAMED(readerThreadValidMemoryRequest);
    // sc_core::sc_signal<bool> SC_NAMED(writerThreadWantsToUseBus);
    // sc_core::sc_signal<bool> SC_NAMED(readerThreadWantsToUseBus);

    const std::uint32_t readsPerCacheline = 0;
    const std::uint32_t cacheLineSize = 0;
    // OrGate SC_NAMED(validMemoryRequestOr);
    // AndGate SC_NAMED(bothThreadsWantToUseBus);

    WriteBuffer(sc_core::sc_module_name name, std::uint32_t readsPerCacheline, std::uint32_t cacheLineSize)
        : sc_module{name}, readsPerCacheline{readsPerCacheline}, cacheLineSize{cacheLineSize} {
        using namespace sc_core;

        //   validMemoryRequestOr.a.bind(writerThreadValidMemoryRequest);
        // validMemoryRequestOr.b.bind(readerThreadValidMemoryRequest);
        // validMemoryRequestOr.out.bind(memoryValidRequestBus);

        SC_THREAD(updateState);
        sensitive << clock.pos();
        SC_THREAD(handleWrite);
        sensitive << clock.neg();
        SC_THREAD(handleRead);
        sensitive << clock.neg();
    }

  private:
    SC_CTOR(WriteBuffer);

    RingQueue<WriteBufferEntry> buffer{SIZE};
    enum class State {
        Write,
        Read,
        Idle,
    };
    State state = State::Idle;

    void writeToRAM() {
        // std::cout << "Write Buffer: Popping off next write " << std::endl;
        assert(buffer.getSize() > 0);
        WriteBufferEntry next = buffer.pop();
        memoryAddrBus.write(next.address);
        memoryDataOutBus.write(next.data);
        memoryWeBus.write(true);
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG
        memoryValidRequestBus.write(true);
        // std::cout << "WriteBuffer: " << "Waiting for RAM write " << std::endl;
        wait(clock.posedge_event());
        do {
            wait(clock.posedge_event());
        } while ((!memoryReadyBus.read()));
        // std::cout << "WriteBuffer: " << "Done waiting for RAM write " << std::endl;
        memoryValidRequestBus.write(false);
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG
        wait(clock.posedge_event());
    }

    void passReadAlong() {
        memoryAddrBus.write(cacheAddrBus.read());
        memoryWeBus.write(false);
        memoryValidRequestBus.write(true);

        // std::cout << "WriteBuffer: " << "Now waiting for RAM" << std::endl;
        do {
            wait(clock.posedge_event());
        } while (!memoryReadyBus.read());
        memoryValidRequestBus.write(false);
        wait(clock.posedge_event());
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG

        //  std::cout << "WriteBuffer: " << " Done waiting for RAM" << std::endl;

        ready.write(true);
        // dont need to wait before first one because we can only get here if RAM tells us it is ready
        for (int i = 0; i < readsPerCacheline; ++i) {
            //  std::cout << "Write buffer received: " << memoryDataInBus.read() << std::endl;
            cacheDataOutBus.write(memoryDataInBus.read());
            wait(clock.posedge_event());
        }

        ready.write(false);
        wait(clock.posedge_event()); // DEBUG
        wait(clock.posedge_event());
    }

    constexpr std::uint32_t makeAddrAligned(std::uint32_t addr) noexcept {
        return (addr / cacheLineSize) * cacheLineSize;
    }

    bool isReadAddrInWriteBuffer(std::uint32_t readAddr) {
        return buffer.any(
            [readAddr, this](WriteBufferEntry& entry) { return makeAddrAligned(entry.address) == readAddr; });
    }

    bool weCanAcceptWriteAndThereIsOne() {
        return (state == State::Idle || state == State::Write) && cacheValidRequest.read() && cacheWeBus.read();
    }

    bool weCanAcceptReadAndThereIsOne() {
        return (state == State::Idle) && cacheValidRequest.read() && !cacheWeBus.read();
    }

    void updateState() {
        while (true) {
            wait();
            if (state == State::Idle || state == State::Write)
                ready.write(false);

            if (weCanAcceptWriteAndThereIsOne()) {

                // add to buffer unless there it is full
                if (buffer.getSize() < SIZE) {
                    std::cout << "Pushing " << buffer.getSize() << std::endl;
                    buffer.push(WriteBufferEntry{cacheAddrBus.read(), cacheDataInBus.read()});
                    ready.write(true);
                    wait();
                } else {
                    std::cout << "NOT READY" << std::endl;
                }
                state = State::Write;
                continue;
            }

            if (weCanAcceptReadAndThereIsOne()) {
                // do read immediately unless the tag is in the buffer
                if (isReadAddrInWriteBuffer(cacheAddrBus.read())) {
                    state = State::Write;
                } else {
                    state = State::Read;
                    continue;
                }
            }

            if (state == State::Idle && (buffer.getSize() > 0)) {
                state = State::Write;
                std::cout << "Writing because was idle" << std::endl;
            }
        }
    }

    void handleRead() {
        while (true) {
            wait();
            if (state == State::Read) {
                passReadAlong();
                state = State::Idle;
            }
        }
    }

    void handleWrite() {
        while (true) {
            wait();
            if (state == State::Write) {
                std::cout << "Writing..." << std::endl;
                writeToRAM();
                std::cout << "Going back to idle" << std::endl;
                state = State::Idle;
            }
        }
    }
};
