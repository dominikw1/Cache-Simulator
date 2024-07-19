#pragma once
#include "RingQueue.h"
#include <cstdint>
#include <systemc>

template <std::uint8_t SIZE> SC_MODULE(WriteBuffer) {
  public:
    // Global -> Buffer
    sc_core::sc_in<bool> SC_NAMED(clock);

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

    const std::uint32_t readsPerCacheline = 0;
    const std::uint32_t cacheLineSize = 0;

    constexpr WriteBuffer(sc_core::sc_module_name name, std::uint32_t readsPerCacheline,
                          std::uint32_t cacheLineSize) noexcept
        : sc_module{name}, readsPerCacheline{readsPerCacheline}, cacheLineSize{cacheLineSize} {
        using namespace sc_core;

        SC_THREAD(updateState);
        sensitive << clock.pos();
        SC_THREAD(handleWrite);
        sensitive << clock.neg();
        SC_THREAD(handleRead);
        sensitive << clock.neg();
    }

  private:
    SC_CTOR(WriteBuffer);

    struct WriteBufferEntry {
        std::uint32_t address;
        std::uint32_t data;
    };

    enum class State {
        Write,
        Read,
        Idle,
    };

    RingQueue<WriteBufferEntry> buffer{SIZE};
    State state = State::Idle;

    constexpr void writeToRAM() noexcept {
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
        do {
            wait();
        } while ((!memoryReadyBus.read()));
        // std::cout << "WriteBuffer: " << "Done waiting for RAM write " << std::endl;
        memoryValidRequestBus.write(false);
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event());
    }

    constexpr void passReadAlong() noexcept {
        memoryAddrBus.write(cacheAddrBus.read());
        memoryWeBus.write(false);
        memoryValidRequestBus.write(true);

        // std::cout << "WriteBuffer: " << "Now waiting for RAM" << std::endl;
        //   wait(clock.posedge_event()); // DEBUG
        do {
            wait();
        } while (!memoryReadyBus.read());
        memoryValidRequestBus.write(false);
        //   wait(clock.posedge_event());
        // wait(clock.posedge_event()); // DEBUG
        // wait(clock.posedge_event()); // DEBUG

        //  std::cout << "WriteBuffer: " << " Done waiting for RAM" << std::endl;

        ready.write(true);
        // dont need to wait before first one because we can only get here if RAM tells us it is ready
        for (std::size_t i = 0; i < readsPerCacheline; ++i) {
            //  std::cout << "Write buffer received: " << memoryDataInBus.read() << std::endl;
            cacheDataOutBus.write(memoryDataInBus.read());
            wait();
        }

        ready.write(false);
        wait(); // this wait is needed because otherwise on rising edge this would instantly be overriden ig??
    }

    constexpr std::uint32_t makeAddrAligned(std::uint32_t addr) noexcept {
        return (addr / cacheLineSize) * cacheLineSize;
    }

    constexpr bool isReadAddrInWriteBuffer(std::uint32_t readAddr) noexcept {
        return buffer.any(
            [readAddr, this](WriteBufferEntry& entry) { return makeAddrAligned(entry.address) == readAddr; });
    }

    constexpr bool weCanAcceptWrite() noexcept { return state == State::Idle || state == State::Write; }
    constexpr bool weCanAcceptRead() noexcept { return state == State::Idle; }
    constexpr bool thereIsAWrite() noexcept { return cacheValidRequest.read() && cacheWeBus.read(); }
    constexpr bool thereIsARead() noexcept { return cacheValidRequest.read() && !cacheWeBus.read(); }

    constexpr void acceptWriteRequest() noexcept {
        if (buffer.getSize() < SIZE) {
            buffer.push(WriteBufferEntry{cacheAddrBus.read(), cacheDataInBus.read()});
            ready.write(true);
            state = State::Write;
            wait(); // otherwise we will add it twice ig?
        } else {
            state = State::Write;
        }
    }

    constexpr void acceptReadRequest() noexcept {
        // do read immediately unless the tag is in the buffer
        if (isReadAddrInWriteBuffer(cacheAddrBus.read())) {
            state = State::Write;
        } else {
            state = State::Read;
        }
    }

    constexpr bool shouldStartNextWrite() noexcept { return state == State::Idle && buffer.getSize() > 0; }

    constexpr void updateState() noexcept {
        while (true) {
            wait();
            if (state != State::Read) // only in read is it possible we are actually ready rn
                ready.write(false);   // this is a kind of catch-all safeguard that we aren't falsely reporting
                                      // readiness. Should never be an issue though

            if (weCanAcceptWrite() && thereIsAWrite()) {
                acceptWriteRequest();
                continue; // we can short - circuit here. We already know what to do next
            }

            if (weCanAcceptRead() && thereIsARead()) {
                acceptReadRequest();
                continue; // we can short - circuit here. We already know what to do next
            }

            if (shouldStartNextWrite()) {
                state = State::Write;
            }
        }
    }

    constexpr void handleRead() noexcept {

        while (true) {
            wait();
            if (state == State::Read) {
              //  std::cout << "WB: Got request at " << sc_core::sc_time_stamp() << "\n";
                passReadAlong();
                state = State::Idle;
                //std::cout << "WB: Done with request at " << sc_core::sc_time_stamp() << "\n";
            }
        }
    }

    constexpr void handleWrite() noexcept {
        while (true) {
            wait();
            if (state == State::Write) {
                //std::cout << "WB: Got write request at " << sc_core::sc_time_stamp() << "\n";
                writeToRAM();
                state = State::Idle;
              //  std::cout << "WB: Done with write request at " << sc_core::sc_time_stamp() << "\n";
            }
        }
    }
};
