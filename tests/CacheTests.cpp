#include "../src/Cache.h"
#include "../src/LRUPolicy.h"
#include "../src/RandomPolicy.h"
#include "../src/Request.h"
#include "Utils.h"
#include <cassert>
#include <exception>
#include <gtest/gtest.h>
#include <iostream>
#include <systemc>
#include <tuple>
#include <unordered_map>
#include <vector>
using namespace sc_core;

SC_MODULE(CPUMock) {
    std::vector<Request> instructions;
    std::vector<Request> instructionsProvided;

    int currPC = 0;

    sc_out<std::uint32_t> addrBus;
    sc_out<std::uint32_t> dataOutBus;
    sc_out<bool> weBus;
    sc_out<bool> validRequestBus;

    sc_in<std::uint32_t> dataInBus;
    sc_in<bool> cacheReadyBus;

    sc_event nextCycleStart;

    void dispatchInstr() {
        while (true) {
            std::cout << " --------------------------- " << std::endl;
            wait(3, SC_NS);
            std::cout << "Providing instruction at " << currPC << std::endl;
            if (instructions.size() <= currPC) {
                std::cout << "Ending simulation" << std::endl;
                return;
            }
            Request requestToWrite = instructions.at(currPC);
            std::cout << "Located request to write" << std::endl;
            weBus.write(requestToWrite.we);
            addrBus.write(requestToWrite.addr);
            dataOutBus.write(requestToWrite.data);
            wait(sc_core::SC_ZERO_TIME);
            validRequestBus.write(true);
            std::cout << "Recording instruction..." << std::endl;
            instructionsProvided.push_back(requestToWrite);
            std::cout << "Done providing instruction" << std::endl;
            wait();
        }
    }

    std::vector<std::pair<std::uint32_t, std::uint32_t>> dataReceivedForAddress;
    void receiveRequestDone() {
        while (true) {
            wait(3, SC_NS);
            std::cout << "Cache request done " << std::endl;
            std::cout << "Logging received data" << std::endl;
            if (instructions.at(currPC).we) {
                dataReceivedForAddress.emplace_back(instructions.at(currPC).addr, instructions.at(currPC).data);
            } else {
                std::cout << "Logging read at " << instructions.at(currPC).addr << " which resulted in "
                          << dataInBus.read() << std::endl;
                dataReceivedForAddress.emplace_back(instructions.at(currPC).addr, dataInBus.read());
            }

            std::cout << "Incrementing pc" << std::endl;
            ++currPC;
            wait(sc_core::SC_ZERO_TIME);
            validRequestBus.write(false);
            nextCycleStart.notify(SC_ZERO_TIME);
            std::cout << "Got past event" << std::endl;
            wait();
        }
    }

    void startUp() { nextCycleStart.notify(SC_ZERO_TIME); }

    SC_CTOR(CPUMock) {
        SC_THREAD(dispatchInstr);
        sensitive << nextCycleStart;
        dont_initialize();

        SC_THREAD(receiveRequestDone);
        sensitive << cacheReadyBus.pos();
        dont_initialize();

        SC_THREAD(startUp);
    }
};

SC_MODULE(RAMMock) {
    sc_in<bool> validDataRequest;
    sc_in<bool> weBus;
    sc_in<std::uint32_t> addressBus;
    std::vector<sc_in<std::uint8_t>> dataInBusses;

    std::vector<sc_out<std::uint8_t>> dataOutBusses;
    sc_out<bool> readyBus;

    std::unordered_map<std::uint32_t, std::uint8_t> dataMemory{};
    std::uint32_t numRequestsPerformed;

    SC_CTOR(RAMMock) {}

    RAMMock(sc_module_name name, unsigned int cacheLineSize)
        : sc_module{name}, dataOutBusses(cacheLineSize), dataInBusses(cacheLineSize) {
        SC_METHOD(provideData);
        sensitive << validDataRequest.pos();
        dont_initialize();

        SC_METHOD(stopProviding);
        sensitive << validDataRequest.neg();
        dont_initialize();

        std::cout << "DataOutBusses size: " << dataOutBusses.size() << std::endl;
        std::cout << "DataInBusses size: " << dataOutBusses.size() << std::endl;
    }

    void stopProviding() { readyBus.write(false); }

    std::uint8_t readByteFromMem(std::uint32_t addr) {
        if (dataMemory.find(addr) == dataMemory.end()) {
            // std::cout << "Defaulting to 0" << std::endl;
            return 0;
        } else {
            // std::cout << "Found in mem" << std::endl;
            return dataMemory[addr];
        }
    }

    void provideData() {
        bool we = weBus.read();
        for (int i = 0; i < 64; ++i) {
            // std::cout << "Doing it for a byte " << std::endl;
            if (!we) {
                //  std::cout << "Reading byte " << i << " as " << unsigned(readByteFromMem(addressBus.read() + i))
                //          << std::endl;
                dataOutBusses.at(i).write(readByteFromMem(addressBus.read() + i));
            } else {
                // std::cout << "Writing byte " << addressBus.read() + i << " as " <<
                // unsigned(dataInBusses.at(i).read())
                //         << std::endl;
                dataMemory[addressBus.read() + i] = dataInBusses.at(i).read();
            }
        }
        readyBus.write(true);
        std::cout << "recording data op" << std::endl;

        /*if (we) {
            std::vector<std::uint8_t> currDataIn{dataInBusses.size()};
            std::transform(dataInBusses.begin(), dataInBusses.end(), currDataIn.begin(),
                           [](sc_in<std::uint8_t>& port) { return port.read(); });
            requestsPerformed.push_back(std::make_pair(addressBus.read(), std::move(currDataIn)));
        } else {
            requestsPerformed.push_back(std::make_pair(addressBus.read(), std::vector<std::uint8_t>{}));
        }*/

        ++numRequestsPerformed;
        std::cout << "Done providing/writing data" << std::endl;
    }
};

class CacheTests : public testing::Test {
  protected:
    CPUMock cpu{"CPU"};
    Cache<MappingType::Fully_Associative> cache{"Cache", 10, 64, 10, std::make_unique<RandomPolicy<std::uint32_t>>(10)};
    RAMMock ram{"RAM", 64};

    // CPU -> Cache
    sc_signal<bool> cpuWeSignal;
    sc_signal<std::uint32_t> cpuAddressSignal;
    sc_signal<std::uint32_t> cpuDataInSignal;
    sc_signal<bool, SC_MANY_WRITERS> cpuValidDataRequestSignal;

    // Cache -> CPU
    sc_signal<std::uint32_t> cpuDataOutSignal;
    sc_signal<bool> cpuDataReadySignal;

    // Cache -> RAM
    sc_signal<bool> ramWeSignal;
    sc_signal<std::uint32_t> ramAdressSignal;
    std::vector<sc_signal<std::uint8_t>> ramDataOutSignals{64};
    sc_signal<bool> ramValidDataRequestSignal;

    // RAM -> Cache
    std::vector<sc_signal<std::uint8_t>> ramDataInSignals{64};
    sc_signal<bool, SC_MANY_WRITERS> ramReadySignal;

    // sc_clock clock{"clk", sc_time(1, SC_NS)};
    void SetUp() {
        //   cpu.clock.bind(clock);
        cpu.weBus.bind(cpuWeSignal);
        cpu.addrBus.bind(cpuAddressSignal);
        cpu.dataOutBus.bind(cpuDataInSignal);
        cpu.validRequestBus.bind(cpuValidDataRequestSignal);

        cpu.dataInBus.bind(cpuDataOutSignal);
        cpu.cacheReadyBus.bind(cpuDataReadySignal);

        cache.cpuAddrBus.bind(cpuAddressSignal);
        cache.cpuDataInBus.bind(cpuDataInSignal);
        cache.cpuWeBus.bind(cpuWeSignal);
        cache.cpuValidRequest.bind(cpuValidDataRequestSignal);

        cache.ready.bind(cpuDataReadySignal);
        cache.cpuDataOutBus.bind(cpuDataOutSignal);

        ram.validDataRequest.bind(ramValidDataRequestSignal);
        ram.weBus.bind(ramWeSignal);
        ram.addressBus.bind(ramAdressSignal);

        assert(64 == ram.dataInBusses.size());
        assert(64 == ram.dataOutBusses.size());
        assert(64 == cache.memoryDataOutBusses.size());
        assert(64 == cache.memoryDataInBusses.size());
        assert(64 == ramDataOutSignals.size());
        assert(64 == ramDataInSignals.size());
        for (int i = 0; i < 64; ++i) {
            ram.dataInBusses.at(i).bind(ramDataOutSignals.at(i));
            ram.dataOutBusses.at(i).bind(ramDataInSignals.at(i));
            cache.memoryDataOutBusses.at(i).bind(ramDataOutSignals.at(i));
            cache.memoryDataInBusses.at(i).bind(ramDataInSignals.at(i));
        }
        ram.readyBus.bind(ramReadySignal);

        cache.memoryAddrBus.bind(ramAdressSignal);
        cache.memoryWeBus.bind(ramWeSignal);
        cache.memoryValidRequestBus.bind(ramValidDataRequestSignal);
        cache.memoryReadyBus.bind(ramReadySignal);
    }
};

TEST_F(CacheTests, CacheTransfersSingleInstructionCorrectly) {
    auto req = Request{1, 5, 0};
    cpu.instructions.push_back(req);
    sc_start(1, SC_SEC);

    ASSERT_EQ(cpu.instructionsProvided.size(), 1);
    ASSERT_EQ(cpu.instructionsProvided.at(0), req);
    ASSERT_EQ(ram.numRequestsPerformed, 1);
    // ASSERT_EQ(ram.numRequestsPerformed.at(0), std::make_pair(0u, std::vector<std::uint8_t>{})); // aligned addr
    ASSERT_EQ(cpu.dataReceivedForAddress.size(), 1);
    ASSERT_EQ(cpu.dataReceivedForAddress.at(0), std::make_pair(1u, 0u));
}

TEST_F(CacheTests, CacheTransfersManyReadInstructionsCorrectly) {
    auto req = std::vector<Request>{Request{1, 5, 0}, Request{2, 60, 0}, Request{100, 2, 0}, Request{919915, 4, 0}};
    for (auto& r : req) {
        cpu.instructions.push_back(r);
    }
    sc_start(2, SC_SEC);

    ASSERT_EQ(cpu.instructionsProvided.size(), req.size());
    ASSERT_EQ(ram.numRequestsPerformed, req.size() - 1); // 1 cache hit
    ASSERT_EQ(cpu.dataReceivedForAddress.size(), req.size());

    for (int i = 0; i < req.size(); ++i) {
        ASSERT_EQ(cpu.instructionsProvided.at(i), req.at(i));
        ASSERT_EQ(cpu.dataReceivedForAddress.at(i), std::make_pair(req.at(i).addr, 0u));
    }
}

TEST_F(CacheTests, CacheTransfersSingleWriteAndSubsequentReadCorrectly) {
    auto req = std::vector<Request>{Request{1, 5, 1}, Request{1, 1, 0}};
    for (auto& r : req) {
        cpu.instructions.push_back(r);
    }
    sc_start(2, SC_SEC);

    ASSERT_EQ(cpu.instructionsProvided.size(), 2);
    ASSERT_EQ(ram.numRequestsPerformed, 2);
    ASSERT_EQ(cpu.dataReceivedForAddress.size(), 2);

    ASSERT_EQ(cpu.instructionsProvided.at(0), req.at(0));
    ASSERT_EQ(cpu.instructionsProvided.at(1), req.at(1));

    ASSERT_EQ(cpu.dataReceivedForAddress.at(1), std::make_pair(1u, 5u));
}

TEST_F(CacheTests, CacheDoesNotThrowWithManyRandomRequests) {
    int numRequests = 100000;
    auto addresses = generateRandomVector(numRequests, UINT32_MAX);
    auto data = generateRandomVector(numRequests, UINT32_MAX);
    auto we = generateRandomVector(numRequests, 1);

    std::vector<Request> requests;
    int numWrites = 0;
    for (int i = 0; i < numRequests; ++i) {
        requests.push_back(Request{addresses.at(i), data.at(i), we.at(i)});
        cpu.instructions.push_back(requests[i]);
        numWrites += we.at(i);
    }

    sc_start(10, SC_SEC);

    ASSERT_EQ(cpu.instructionsProvided.size(), numRequests);
    // due to unaligned accesses it might be larger
    ASSERT_GE(ram.numRequestsPerformed, numRequests + numWrites - cache.hitCount);
    // but no larger than twice the amount of requests -> there is no case where we need three cachelines per request
    ASSERT_LE(ram.numRequestsPerformed, 2 * (numRequests + numWrites - cache.hitCount));
}

TEST_F(CacheTests, CacheReadReturnsSameValueAsWrittenBefore) {
    int numRequests = 10000;
    auto addresses = generateRandomVector(numRequests, UINT32_MAX);
    auto data = generateRandomVector(numRequests, UINT32_MAX);

    std::vector<Request> requests;
    for (int i = 0; i < numRequests; ++i) {
        std::uint32_t currData = data.at(i);
        cpu.instructions.push_back(Request{addresses.at(i), currData, 1});
        cpu.instructions.push_back(Request{addresses.at(i), 0, 0});
    }
    sc_start(10, SC_SEC);

    ASSERT_EQ(cpu.instructionsProvided.size(), 2 * numRequests);
    for (int i = 0; i < numRequests; ++i) {
        std::cout << std::get<1>(cpu.dataReceivedForAddress.at(2 * i + 1)) << " "
                  << std::get<1>(cpu.dataReceivedForAddress.at(2 * i)) << " "
                  << std::get<0>(cpu.dataReceivedForAddress.at(2 * i)) << std::endl;
        ASSERT_EQ(std::get<0>(cpu.dataReceivedForAddress.at(2 * i)), addresses.at(i));
        ASSERT_EQ(std::get<1>(cpu.dataReceivedForAddress.at(2 * i)),
                  std::get<1>(cpu.dataReceivedForAddress.at(2 * i + 1)));
        ASSERT_EQ(std::get<1>(cpu.dataReceivedForAddress.at(2 * i)), data.at(i));
    }
}

TEST(CacheHelperTests, SubRequestSplitterDoesNotChangeAlignedRequest) {
    auto subReqs = splitRequestIntoSubRequests(Request{0, 10, 1}, 100);

    ASSERT_EQ(subReqs.size(), 1);
    ASSERT_EQ(subReqs[0].bitsBefore, 0);
    ASSERT_EQ(subReqs[0].addr, 0);
    ASSERT_EQ(subReqs[0].data, 10);
    ASSERT_EQ(subReqs[0].size, 4);
    ASSERT_EQ(subReqs[0].we, true);
}

TEST(CacheHelperTests, SubRequestSplitterDoesNotChangeAlignedRequestAtBorder) {
    auto subReqs = splitRequestIntoSubRequests(Request{100 - 4 - 1, 10, 1}, 100);
    ASSERT_EQ(subReqs.size(), 1);
    ASSERT_EQ(subReqs[0].bitsBefore, 0);
    ASSERT_EQ(subReqs[0].addr, 100 - 4 - 1);
    ASSERT_EQ(subReqs[0].data, 10);
    ASSERT_EQ(subReqs[0].size, 4);
    ASSERT_EQ(subReqs[0].we, true);
}

TEST(CacheHelperTests, SubRequestSplitterDoesNotChangeAlignedRequestNotAtFirstLine) {
    auto subReqs = splitRequestIntoSubRequests(Request{200 - 4 - 1, 10, 1}, 100);
    ASSERT_EQ(subReqs.size(), 1);
    ASSERT_EQ(subReqs[0].bitsBefore, 0);
    ASSERT_EQ(subReqs[0].addr, 200 - 4 - 1);
    ASSERT_EQ(subReqs[0].data, 10);
    ASSERT_EQ(subReqs[0].size, 4);
    ASSERT_EQ(subReqs[0].we, true);
}

TEST(CacheHelperTests, SubRequestSplitterDoesNotChangeAlignedRequestMaxNum) {
    auto subReqs = splitRequestIntoSubRequests(Request{200 - 4 - 1, UINT32_MAX, 1}, 100);
    ASSERT_EQ(subReqs.size(), 1);
    ASSERT_EQ(subReqs[0].bitsBefore, 0);
    ASSERT_EQ(subReqs[0].addr, 200 - 4 - 1);
    ASSERT_EQ(subReqs[0].data, UINT32_MAX);
    ASSERT_EQ(subReqs[0].size, 4);
    ASSERT_EQ(subReqs[0].we, true);
}

TEST(CacheHelperTests, SubRequestSplitterCorrectlySplitsInTrivialCase) {
    auto subReqs = splitRequestIntoSubRequests(Request{4, 0, 1}, 1);
    ASSERT_EQ(subReqs.size(), 4);
    for (int i = 0; i < 4; ++i) {
        ASSERT_EQ(subReqs[i].addr, 4 + i);
        ASSERT_EQ(subReqs[i].size, 1);
        ASSERT_EQ(subReqs[i].data, 0);
        ASSERT_EQ(subReqs[i].we, true);
    }
}

TEST(CacheHelperTests, SubRequestSplitterCorrectlySplitsCacheline16Bit) {
    auto subReqs = splitRequestIntoSubRequests(Request{5, 8, 0}, 2);
    ASSERT_EQ(subReqs.size(), 3);
    ASSERT_EQ(subReqs[0].bitsBefore, 0);
    ASSERT_EQ(subReqs[1].bitsBefore, 8);
    ASSERT_EQ(subReqs[2].bitsBefore, 24);
    ASSERT_EQ(subReqs[0].addr, 5);
    ASSERT_EQ(subReqs[1].addr, 6);
    ASSERT_EQ(subReqs[2].addr, 8);
    ASSERT_EQ(subReqs[0].size, 1);
    ASSERT_EQ(subReqs[1].size, 2);
    ASSERT_EQ(subReqs[2].size, 1);
    ASSERT_EQ(subReqs[0].data, 8);
    ASSERT_EQ(subReqs[1].data, 0);
    ASSERT_EQ(subReqs[2].data, 0);
    ASSERT_EQ(subReqs[0].we, false);
    ASSERT_EQ(subReqs[1].we, false);
    ASSERT_EQ(subReqs[2].we, false);
}

TEST_F(CacheTests, CacheTestReadWriteReturnsSameInFailureCase) {
    std::uint32_t problematicVal = 1322427197u;
    Request w{problematicVal, problematicVal, 1};
    Request r{problematicVal, problematicVal, 0};
    cpu.instructions.push_back(w);
    cpu.instructions.push_back(r);

    sc_start(2, SC_SEC);
    ASSERT_EQ(cpu.dataReceivedForAddress.size(), 2);
    ASSERT_EQ(std::get<1>(cpu.dataReceivedForAddress.at(0)), std::get<1>(cpu.dataReceivedForAddress.at(1)));
    ASSERT_EQ(std::get<1>(cpu.dataReceivedForAddress.at(0)), problematicVal);
}

TEST_F(CacheTests, CacheRightNumberHitsForOnlyReadsOnSameAddr) {
    std::uint32_t addr = 15915959ull;

    Request r{addr, 0, 0};
    int numRequests = 10000;
    for (std::size_t i = 0; i < numRequests; ++i) {
        cpu.instructions.push_back(r);
    }

    sc_start(2, SC_SEC);
    ASSERT_EQ(cpu.dataReceivedForAddress.size(), numRequests);
    ASSERT_EQ(cache.hitCount, numRequests - 1);
    ASSERT_EQ(cache.missCount, 1);
}

TEST_F(CacheTests, CacheNumberRightHitsAllReadsUnalignedAddress) {
    std::uint32_t addr = 62;

    Request r{addr, 0, 0};
    int numRequests = 10000;
    for (std::size_t i = 0; i < numRequests; ++i) {
        cpu.instructions.push_back(r);
    }

    sc_start(2, SC_SEC);
    ASSERT_EQ(cpu.dataReceivedForAddress.size(), numRequests);
    ASSERT_EQ(cache.hitCount, 2 * numRequests - 2);
    ASSERT_EQ(cache.missCount, 2);
}

TEST_F(CacheTests, CacheNumberRightHitsMixedReadWritesIndependentAligned) {
    std::uint32_t addr1 = 59;
    std::uint32_t addr2 = 4 + 64;
    std::uint32_t addr3 = 20 + 64 * 2;

    Request w1{addr1, 5, 0};
    Request w2{addr2, 10, 0};
    Request w3{addr3, 23, 0};
    Request r1{addr1, 0, 0};
    Request r2{addr2, 0, 0};
    Request r3{addr3, 0, 0};

    cpu.instructions.push_back(w1); // each write is 1 miss
    cpu.instructions.push_back(w2);
    cpu.instructions.push_back(w3);

    cpu.instructions.push_back(r1); // each read is one hit
    cpu.instructions.push_back(r2);
    cpu.instructions.push_back(r3);
    cpu.instructions.push_back(r3);
    cpu.instructions.push_back(r3);

    sc_start(2, SC_SEC);
    ASSERT_EQ(cpu.dataReceivedForAddress.size(), 8);
    ASSERT_EQ(cache.hitCount, 5);
    ASSERT_EQ(cache.missCount, 3);
}
