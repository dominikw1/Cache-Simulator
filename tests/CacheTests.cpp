#include "../src/Request.h"
#include "../src/Simulation/Cache.h"
#include "../src/Simulation/Policy/LRUPolicy.h"
#include "../src/Simulation/Policy/RandomPolicy.h"
#include "Utils.h"
#include <cassert>
#include <exception>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <systemc>
#include <tuple>
#include <unordered_map>
#include <vector>
using namespace sc_core;

SC_MODULE(CPUMock) {
    std::vector<Request> instructions;
    std::vector<Request> instructionsProvided;

    std::uint64_t currPC = 0;

    sc_out<std::uint32_t> addrBus;
    sc_out<std::uint32_t> dataOutBus;
    sc_out<bool> weBus;
    sc_out<bool> validRequestBus;

    sc_in<std::uint32_t> dataInBus;
    sc_in<bool> cacheReadyBus;

    sc_in<bool> clock;
    sc_event nextCycleStart;

    void dispatchInstr() {
        while (true) {
            wait();
            if (instructions.size() <= currPC) {
                std::cout << "Ending simulation" << std::endl;
                return;
            }
            Request requestToWrite = instructions.at(currPC);
            weBus.write(requestToWrite.we);
            addrBus.write(requestToWrite.addr);
            dataOutBus.write(requestToWrite.data);
            validRequestBus.write(true);

            instructionsProvided.push_back(requestToWrite);

            wait();
            validRequestBus.write(false);
            while (!cacheReadyBus) {
                wait();
            }

            if (instructions.at(currPC).we) {
                dataReceivedForAddress.emplace_back(instructions.at(currPC).addr, instructions.at(currPC).data);
            } else {
                dataReceivedForAddress.emplace_back(instructions.at(currPC).addr, dataInBus.read());
            }

            ++currPC;
        }
    }

    std::vector<std::pair<std::uint32_t, std::uint32_t>> dataReceivedForAddress;

    SC_CTOR(CPUMock) {
        SC_THREAD(dispatchInstr);
        sensitive << clock.pos();
    }
};

SC_MODULE(RAMMock) {

    sc_in<bool> clock;

    sc_in<bool> validDataRequest;
    sc_in<bool> weBus;
    sc_in<std::uint32_t> addressBus;
    sc_in<std::uint32_t> dataInBus;

    sc_out<sc_dt::sc_bv<128>> dataOutBus;
    sc_out<bool> readyBus;

    std::map<std::uint32_t, std::uint8_t> dataMemory{};
    std::uint32_t numRequestsPerformed;

    SC_CTOR(RAMMock) {}
    std::uint32_t latency = 20;
    std::uint32_t wordsPerRead;
    RAMMock(sc_module_name name, std::uint32_t wordsPerRead) : sc_module{name}, wordsPerRead{wordsPerRead} {
        SC_THREAD(provideData);
        sensitive << clock.pos();
        dont_initialize();
    }

    std::uint8_t readByteFromMem(std::uint32_t addr) { return dataMemory[addr]; }

    void provideData() {
        while (true) {
            wait(clock.posedge_event());
            readyBus.write(false);

            if (!validDataRequest.read())
                continue;

            // Wait out latency
            for (std::size_t cycles = 0; cycles < latency; ++cycles) {
                wait(clock.posedge_event());
            }

            if (weBus.read()) {
                // Writing happens in one cycle -> one able to write 32 bits
                dataMemory[addressBus.read()] = (dataInBus.read() & ((1 << 8) - 1));
                dataMemory[addressBus.read() + 1] = (dataInBus.read() >> 8) & ((1 << 8) - 1);
                dataMemory[addressBus.read() + 2] = (dataInBus.read() >> 16) & ((1 << 8) - 1);
                dataMemory[addressBus.read() + 3] = (dataInBus.read() >> 24) & ((1 << 8) - 1);
                readyBus.write(true);
            } else {
                // Reading takes wordsPerRead cycles
                sc_dt::sc_bv<128> readData;
                for (std::uint32_t i = 0; i < wordsPerRead; ++i) {
                    // 128 / 8 -> 16
                    for (int byte = 0; byte < 16; ++byte) {
                        readData.range(8 * byte + 7, 8 * byte) = readByteFromMem(addressBus.read() + i * 16 + byte);
                    }
                    dataOutBus.write(readData);

                    // Next word is ready and then wait for next cycle to continue reading
                    readyBus.write(true);

                    if (i != wordsPerRead - 1) {
                        wait(clock.posedge_event());
                    }
                }
            }

            ++numRequestsPerformed;
        }
    }
};

template <typename T> class CacheTests : public testing::Test {
  public:
    CPUMock cpu{"CPU"};
    Cache<T::value> cache{"Cache", 10, 64, 10, std::make_unique<RandomPolicy<std::uint32_t>>(10)};
    RAMMock ram{"RAM", 64 / 16};

    // CPU -> Cache
    sc_signal<bool> SC_NAMED(cpuWeSignal);
    sc_signal<std::uint32_t> SC_NAMED(cpuAddressSignal);
    sc_signal<std::uint32_t> SC_NAMED(cpuDataInSignal);
    sc_signal<bool, SC_MANY_WRITERS> SC_NAMED(cpuValidDataRequestSignal);

    // Cache -> CPU
    sc_signal<std::uint32_t> SC_NAMED(cpuDataOutSignal);
    sc_signal<bool> SC_NAMED(cpuDataReadySignal);

    // Cache -> RAM
    sc_signal<bool, SC_MANY_WRITERS> SC_NAMED(ramWeSignal);
    sc_signal<std::uint32_t, SC_MANY_WRITERS> SC_NAMED(ramAdressSignal);
    sc_signal<sc_dt::sc_bv<128>> SC_NAMED(ramDataOutSignal);
    sc_signal<bool, SC_MANY_WRITERS> SC_NAMED(ramValidDataRequestSignal);

    // RAM -> Cache
    sc_signal<std::uint32_t> SC_NAMED(ramDataInSignal);
    sc_signal<bool, SC_MANY_WRITERS> SC_NAMED(ramReadySignal);

    sc_clock clock{"clk", sc_time(1, SC_NS)};
    void SetUp() override {
        ram.latency = 20;
#ifdef STRICT_INSTRUCTION_ORDER
        cache.setMemoryLatency(ram.latency);
#endif
        cpu.clock.bind(clock);
        cpu.weBus.bind(cpuWeSignal);
        cpu.addrBus.bind(cpuAddressSignal);
        cpu.dataOutBus.bind(cpuDataInSignal);
        cpu.validRequestBus.bind(cpuValidDataRequestSignal);

        cpu.dataInBus.bind(cpuDataOutSignal);
        cpu.cacheReadyBus.bind(cpuDataReadySignal);

        cache.clock.bind(clock);

        cache.cpuAddrBus.bind(cpuAddressSignal);
        cache.cpuDataInBus.bind(cpuDataInSignal);
        cache.cpuWeBus.bind(cpuWeSignal);
        cache.cpuValidRequest.bind(cpuValidDataRequestSignal);

        cache.ready.bind(cpuDataReadySignal);
        cache.cpuDataOutBus.bind(cpuDataOutSignal);

        ram.validDataRequest.bind(ramValidDataRequestSignal);
        ram.weBus.bind(ramWeSignal);
        ram.addressBus.bind(ramAdressSignal);
        ram.dataInBus.bind(ramDataInSignal);

        ram.readyBus.bind(ramReadySignal);
        ram.dataOutBus.bind(ramDataOutSignal);
        ram.clock.bind(clock);

        cache.memoryDataInBus.bind(ramDataOutSignal);
        cache.memoryAddrBus.bind(ramAdressSignal);
        cache.memoryWeBus.bind(ramWeSignal);
        cache.memoryValidRequestBus.bind(ramValidDataRequestSignal);

        cache.memoryDataOutBus.bind(ramDataInSignal);
        cache.memoryReadyBus.bind(ramReadySignal);
    }
};

template <MappingType mappingType> struct TestMappingType {
    static constexpr MappingType value = mappingType;
};

using MappingTypes =
    ::testing::Types<TestMappingType<MappingType::Direct>, TestMappingType<MappingType::Fully_Associative>>;

TYPED_TEST_SUITE(CacheTests, MappingTypes);

TYPED_TEST(CacheTests, CacheTransfersSingleInstructionCorrectly) {
    auto req = Request{1, 5, 0};
    TestFixture::cpu.instructions.push_back(req);
    sc_start(1, SC_MS);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), 1);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.at(0), req);
    ASSERT_EQ(TestFixture::ram.numRequestsPerformed, 1);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), 1);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.at(0), std::make_pair(1u, 0u));
}

TYPED_TEST(CacheTests, CacheTransfersManyReadInstructionsCorrectly) {
    auto req = std::vector<Request>{Request{1, 5, 0}, Request{2, 60, 0}, Request{100, 2, 0}, Request{919915, 4, 0}};
    for (auto& r : req) {
        TestFixture::cpu.instructions.push_back(r);
    }
    sc_start(2, SC_MS);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), req.size());
    ASSERT_EQ(TestFixture::ram.numRequestsPerformed, req.size() - 1); // 1 cache hit
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), req.size());
    for (std::size_t i = 0; i < req.size(); ++i) {
        ASSERT_EQ(TestFixture::cpu.instructionsProvided.at(i), req.at(i));
        ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.at(i), std::make_pair(req.at(i).addr, 0u));
    }
}

TYPED_TEST(CacheTests, CacheTransfersSingleWriteAndSubsequentReadCorrectly) {
    auto req = std::vector<Request>{Request{1, 5, 1}, Request{1, 1, 0}};
    for (auto& r : req) {
        TestFixture::cpu.instructions.push_back(r);
    }
    sc_start(2, SC_MS);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), 2);
    ASSERT_EQ(TestFixture::ram.numRequestsPerformed, 2);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), 2);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.at(0), req.at(0));
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.at(1), req.at(1));
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.at(1), std::make_pair(1u, 5u));
}

TYPED_TEST(CacheTests, CacheDoesNotThrowWithManyRandomRequests) {
    TestFixture::ram.latency = 1;
    int numRequests = 1000;
    auto addresses = generateRandomVector(numRequests, UINT32_MAX);
    auto data = generateRandomVector(numRequests, UINT32_MAX);
    auto we = generateRandomVector(numRequests, 1);

    std::vector<Request> requests;
    int numWrites = 0;
    for (int i = 0; i < numRequests; ++i) {
        requests.push_back(Request{static_cast<std::uint32_t>(addresses.at(i)), static_cast<std::uint32_t>(data.at(i)),
                                   static_cast<int>(we.at(i))});
        TestFixture::cpu.instructions.push_back(requests[i]);
        numWrites += static_cast<int>(we.at(i));
    }

    sc_start(3, SC_MS);

    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), numRequests);
    // due to unaligned accesses it might be larger
    ASSERT_GE(TestFixture::ram.numRequestsPerformed, numRequests + numWrites - TestFixture::cache.hitCount);
    // but no larger than twice the amount of requests -> there is no case where we need three cachelines per request
    ASSERT_LE(TestFixture::ram.numRequestsPerformed, 2 * (numRequests + numWrites - TestFixture::cache.hitCount));
}

TYPED_TEST(CacheTests, CacheReadReturnsSameValueAsWrittenBefore) {
    int numRequests = 1000;
    auto addresses = generateRandomVector(numRequests, UINT32_MAX);
    auto data = generateRandomVector(numRequests, UINT32_MAX);

    std::vector<Request> requests;
    for (int i = 0; i < numRequests; ++i) {
        std::uint32_t currData = data.at(i);
        TestFixture::cpu.instructions.push_back(Request{static_cast<uint32_t>(addresses.at(i)), currData, 1});
        TestFixture::cpu.instructions.push_back(Request{static_cast<uint32_t>(addresses.at(i)), 0, 0});
    }
    sc_start(10, SC_MS);

    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), 2 * numRequests);
    for (int i = 0; i < numRequests; ++i) {
        ASSERT_EQ(std::get<0>(TestFixture::cpu.dataReceivedForAddress.at(2 * i)), addresses.at(i));
        ASSERT_EQ(std::get<1>(TestFixture::cpu.dataReceivedForAddress.at(2 * i)),
                  std::get<1>(TestFixture::cpu.dataReceivedForAddress.at(2 * i + 1)));
        ASSERT_EQ(std::get<1>(TestFixture::cpu.dataReceivedForAddress.at(2 * i)), data.at(i));
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

TYPED_TEST(CacheTests, CacheTestReadWriteReturnsSameInFailureCase) {
    std::uint32_t problematicVal = 1322427197u;
    Request w{problematicVal, problematicVal, 1};
    Request r{problematicVal, problematicVal, 0};
    TestFixture::cpu.instructions.push_back(w);
    TestFixture::cpu.instructions.push_back(r);

    sc_start(2, SC_MS);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), 2);
    ASSERT_EQ(std::get<1>(TestFixture::cpu.dataReceivedForAddress.at(0)),
              std::get<1>(TestFixture::cpu.dataReceivedForAddress.at(1)));
    ASSERT_EQ(std::get<1>(TestFixture::cpu.dataReceivedForAddress.at(0)), problematicVal);
}

TYPED_TEST(CacheTests, CacheRightNumberHitsForOnlyReadsOnSameAddr) {
    std::uint32_t addr = 15915959ull;

    Request r{addr, 0, 0};
    int numRequests = 10000;
    for (int i = 0; i < numRequests; ++i) {
        TestFixture::cpu.instructions.push_back(r);
    }

    sc_start(2, SC_MS);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), numRequests);
    ASSERT_EQ(TestFixture::cache.hitCount, numRequests - 1);
    ASSERT_EQ(TestFixture::cache.missCount, 1);
}

TYPED_TEST(CacheTests, CacheNumberRightHitsAllReadsUnalignedAddress) {
    std::uint32_t addr = 62;

    Request r{addr, 0, 0};
    int numRequests = 10000;
    for (int i = 0; i < numRequests; ++i) {
        TestFixture::cpu.instructions.push_back(r);
    }

    sc_start(2, SC_MS);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), numRequests);
    ASSERT_EQ(TestFixture::cache.hitCount, 2 * numRequests - 2);
    ASSERT_EQ(TestFixture::cache.missCount, 2);
}

TYPED_TEST(CacheTests, CacheNumberRightHitsMixedReadWritesIndependentAligned) {
    std::uint32_t addr1 = 59;
    std::uint32_t addr2 = 4 + 64;
    std::uint32_t addr3 = 20 + 64 * 2;

    Request w1{addr1, 5, 1};
    Request w2{addr2, 10, 1};
    Request w3{addr3, 23, 1};
    Request r1{addr1, 0, 0};
    Request r2{addr2, 0, 0};
    Request r3{addr3, 0, 0};

    TestFixture::cpu.instructions.push_back(w1); // each write is 1 miss
    TestFixture::cpu.instructions.push_back(w2);
    TestFixture::cpu.instructions.push_back(w3);

    TestFixture::cpu.instructions.push_back(r1); // each read is one hit
    TestFixture::cpu.instructions.push_back(r2);
    TestFixture::cpu.instructions.push_back(r3);
    TestFixture::cpu.instructions.push_back(r3);
    TestFixture::cpu.instructions.push_back(r3);

    sc_start(2, SC_MS);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), 8);
    ASSERT_EQ(TestFixture::cache.hitCount, 5);
    ASSERT_EQ(TestFixture::cache.missCount, 3);
}

TYPED_TEST(CacheTests, CacheReadsCorrectValueAfterUnalignedWrite) {
    std::uint32_t addr1 = 60;
    std::uint32_t addr2 = 62;
    std::uint32_t addr3 = 64;

    std::uint32_t data1 = 591591;
    std::uint32_t data2 = UINT32_MAX - 1052ull;
    std::uint32_t data3 = 1002017350;

    std::uint32_t expectedData1 = (data1 & ((1 << 16) - 1)) | ((data2 & ((1 << 16) - 1)) << 16);
    std::uint32_t expectedData2 = data2;
    std::uint32_t expectedData3 = ((data2 >> 16) & ((1 << 16) - 1)) | (data3 & (~((1 << 16) - 1)));

    Request w1{addr1, data1, 1};
    Request w2{addr2, data2, 1};
    Request w3{addr3, data3, 1};
    Request r1{addr1, 0, 0};
    Request r2{addr2, 0, 0};
    Request r3{addr3, 0, 0};

    TestFixture::cpu.instructions.push_back(w1);
    TestFixture::cpu.instructions.push_back(w3);
    TestFixture::cpu.instructions.push_back(w2);

    TestFixture::cpu.instructions.push_back(r1);
    TestFixture::cpu.instructions.push_back(r2);
    TestFixture::cpu.instructions.push_back(r3);

    sc_start(2, SC_MS);

    auto readForLocation1 = *(TestFixture::cpu.dataReceivedForAddress.end() - 3);
    auto readForLocation2 = *(TestFixture::cpu.dataReceivedForAddress.end() - 2);
    auto readForLocation3 = *(TestFixture::cpu.dataReceivedForAddress.end() - 1);

    ASSERT_EQ(std::get<0>(readForLocation1), addr1);
    ASSERT_EQ(std::get<0>(readForLocation2), addr2);
    ASSERT_EQ(std::get<0>(readForLocation3), addr3);
    ASSERT_EQ(std::get<1>(readForLocation1), expectedData1);
    ASSERT_EQ(std::get<1>(readForLocation2), expectedData2);
    ASSERT_EQ(std::get<1>(readForLocation3), expectedData3);
}

TYPED_TEST(CacheTests, CacheSingleWriteUsesWriteBuffer) {
    TestFixture::ram.latency = 1000;
    Request writeRequest{10, 100, 1};
    Request readRequest{10, 100, 0};
    TestFixture::cpu.instructions.push_back(writeRequest);
    TestFixture::cpu.instructions.push_back(readRequest);

    sc_start(1000 + 100, SC_NS);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), 2);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), 2);
    ASSERT_EQ(TestFixture::ram.numRequestsPerformed, 1); // only the read
}

TEST(AddrDecompTests, ceilLog2WorksFor1) { ASSERT_EQ(safeCeilLog2(1), 0); }

TYPED_TEST(CacheTests, CacheMultiWriteBuffersIfSameCacheline) {
    TestFixture::ram.latency = 1000;
    Request writeRequest1{10, 100, 1};
    Request writeRequest2{20, 100, 1};
    Request writeRequest3{1, 914919, 1};
    Request readRequest{10, 100, 0};

    TestFixture::cpu.instructions.push_back(writeRequest1);
    TestFixture::cpu.instructions.push_back(writeRequest2);
    TestFixture::cpu.instructions.push_back(writeRequest3);
    TestFixture::cpu.instructions.push_back(readRequest);

    // 1000 for first read, then write to buffer (~instantly), then <100 for each next request to supply
    sc_start(2000, SC_NS);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), 4);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), 4);
    ASSERT_EQ(TestFixture::ram.numRequestsPerformed, 1); // only the read
}

TYPED_TEST(CacheTests, CacheWriteBufferReadDoesNotWaitForWrites) {
    TestFixture::ram.latency = 1000;
    Request writeRequest1{10, 100, 1};
    Request writeRequest2{20, 100, 1};
    Request readRequest{51591, 100, 0};
    Request writeRequest3{1, 914919, 1};

    TestFixture::cpu.instructions.push_back(writeRequest1);
    TestFixture::cpu.instructions.push_back(writeRequest2);
    TestFixture::cpu.instructions.push_back(readRequest); // will really be executed after first write
    TestFixture::cpu.instructions.push_back(writeRequest3);

    sc_start(3200, SC_NS);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), 4);
    ASSERT_EQ(TestFixture::cpu.dataReceivedForAddress.size(), 4);
    ASSERT_EQ(TestFixture::ram.numRequestsPerformed, 3); // first read, first write, second read
}

TYPED_TEST(CacheTests, CacheWriteBufferHandlesMoreWritesThanCapacity) {
    TestFixture::ram.latency = 500;
    Request writeRequest1{10, 100, 1};
    Request writeRequest2{20, 100, 1};
    Request writeRequest3{1, 914919, 1};
    Request writeRequest4{5, 100, 1};
    Request writeRequest5{50, 100, 1};
    Request writeRequest6{40, 914919, 1};
    TestFixture::cpu.instructions.push_back(writeRequest1);
    TestFixture::cpu.instructions.push_back(writeRequest2);
    TestFixture::cpu.instructions.push_back(writeRequest3);
    TestFixture::cpu.instructions.push_back(writeRequest4);
    TestFixture::cpu.instructions.push_back(writeRequest5);
    TestFixture::cpu.instructions.push_back(writeRequest6);

    sc_start(10, SC_MS);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), 6);
    ASSERT_EQ(TestFixture::ram.numRequestsPerformed, 7); // 6 writes + 1 read
}

TYPED_TEST(CacheTests, CacheReadsResultInSameValuesAsManuallyRecorded) {
    TestFixture::ram.latency = 1;
    int numRequests = 10000;
    auto addresses = generateRandomVector(numRequests, UINT32_MAX);
    auto data = generateRandomVector(numRequests, UINT32_MAX);
    std::unordered_map<std::uint32_t, std::uint8_t> memRecord;

    std::vector<Request> requests;
    for (int i = 0; i < numRequests; ++i) {
        std::uint32_t currData = data.at(i);
        TestFixture::cpu.instructions.push_back(Request{static_cast<uint32_t>(addresses.at(i)), currData, 1});
        memRecord[addresses.at(i) + 0] = (currData >> 0) & ((1 << 8) - 1);
        memRecord[addresses.at(i) + 1] = (currData >> 8) & ((1 << 8) - 1);
        memRecord[addresses.at(i) + 2] = (currData >> 16) & ((1 << 8) - 1);
        memRecord[addresses.at(i) + 3] = (currData >> 24) & ((1 << 8) - 1);
    }

    for (int i = 0; i < numRequests; ++i) {
        TestFixture::cpu.instructions.push_back(Request{static_cast<uint32_t>(addresses.at(i)), 0, 0});
    }

    sc_start(2, SC_MS);
    ASSERT_EQ(TestFixture::cpu.instructionsProvided.size(), 2 * numRequests);

    for (int i = 0; i < numRequests; ++i) {
        auto addrRead = std::get<0>(TestFixture::cpu.dataReceivedForAddress.at(numRequests + i));
        auto dataRead = std::get<1>(TestFixture::cpu.dataReceivedForAddress.at(numRequests + i));
        ASSERT_EQ(addrRead, addresses.at(i));
        ASSERT_EQ(memRecord[addrRead], dataRead & ((1 << 8) - 1));
        ASSERT_EQ(memRecord[addrRead + 1], (dataRead >> 8) & ((1 << 8) - 1));
        ASSERT_EQ(memRecord[addrRead + 2], (dataRead >> 16) & ((1 << 8) - 1));
        ASSERT_EQ(memRecord[addrRead + 3], (dataRead >> 24) & ((1 << 8) - 1));
    }
}

TEST(HelperTest, SaturatingAddWorks) {
    ASSERT_EQ(addSatUnsigned(UINT32_MAX, (uint32_t)9499), UINT32_MAX);
    ASSERT_EQ(addSatUnsigned(4u, 5u), 9u);
}

TEST(HelperTest, SaturatingMulWorks) {
    ASSERT_EQ(mulSatUnsigned(UINT32_MAX, (uint32_t)9499), UINT32_MAX);
    ASSERT_EQ(mulSatUnsigned(4u, 5u), 20u);
}

TEST(HelperTest, SaturatingPowWorks) {
    ASSERT_EQ(powSatUnsigned(UINT32_MAX, (uint32_t)9499), UINT32_MAX);
    ASSERT_EQ(powSatUnsigned(2u, 8u), 1u << 8u);
}