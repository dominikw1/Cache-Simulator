#include <gtest/gtest.h>
#include <systemc>
#include "../src/Memory.h"
#include "../src/InstructionCache.h"
#include "../src/CPU.h"
#include "../src/Policy/Policy.h"
#include "../src/Policy/FIFOPolicy.h"
#include "../src/Policy/LRUPolicy.h"
#include "../src/Policy/RandomPolicy.h"
#include "Utils.h"

using namespace sc_core;

struct Connections {
    sc_clock clk{"Clock", sc_time(1, SC_NS)};

    // Data Cache
    // CPU -> Cache
    sc_signal<bool> cpuWeSignal;
    sc_signal<std::uint32_t> cpuAddressSignal;
    sc_signal<std::uint32_t> cpuDataOutSignal;
    sc_signal<bool> cpuValidDataRequestSignal;

    // Cache -> CPU
    sc_signal<std::uint32_t> cpuDataInSignal;
    sc_signal<bool> cpuDataReadySignal;

    // Cache -> RAM
    sc_signal<bool, SC_MANY_WRITERS> ramWeSignal;
    sc_signal<std::uint32_t, SC_MANY_WRITERS> ramAddressSignal;
    sc_signal<std::uint32_t> ramDataInSignal;
    sc_signal<bool, SC_MANY_WRITERS> ramValidRequestSignal;

    // RAM -> Cache
    sc_signal<sc_dt::sc_bv<128>> ramDataOutSignal;
    sc_signal<bool> ramReadySignal;

    // Instruction Cache
    // CPU -> Cache
    sc_signal<bool> validInstrRequestSignal;
    sc_signal<std::uint32_t> pcSignal;

    // Cache -> CPU
    sc_signal<Request> instructionSignal;
    sc_signal<bool> instrReadySignal;

    // Cache -> RAM
    sc_signal<std::uint32_t> instrRamAddressSignal;
    sc_signal<bool> instrRamWeBus;
    sc_signal<bool> instrRamValidRequestBus;
    sc_signal<std::uint32_t> instrRamDataInBus;

    // RAM -> Cache
    sc_signal<sc_dt::sc_bv<128>> instrRamDataOutSignal;
    sc_signal<bool> instrRamReadySignal;
};

template<MappingType mappingType>
std::unique_ptr<Connections> connectComp(CPU& cpu,
                                         RAM& dataRam, RAM& instructionRam,
                                         Cache<mappingType>& dataCache, InstructionCache& instructionCache);

std::unique_ptr<ReplacementPolicy<std::uint32_t>>
getReplacementPolity(CacheReplacementPolicy replacementPolicy, unsigned int cacheSize);

std::vector<Request> generateRandomRequests(std::uint64_t len);

class IntegrationTests
        : public ::testing::TestWithParam<std::tuple<unsigned int, unsigned int, unsigned int, unsigned int, CacheReplacementPolicy, std::vector<Request>>> {
protected:
    unsigned int memoryLatency = std::get<0>(GetParam());
    unsigned int cacheLatency = std::get<1>(GetParam());
    unsigned int cacheLines = std::get<2>(GetParam());
    unsigned int cacheLineSize = std::get<3>(GetParam());
    CacheReplacementPolicy policy = std::get<4>(GetParam());
    std::vector<Request> requests = std::get<5>(GetParam());

    CPU cpu{"CPU", requests.size()};

    RAM dataRam{"Data_RAM", cacheLatency, cacheLatency};
    RAM instructionRam{"Instruction_RAM", memoryLatency, 64};

    InstructionCache instructionCache{"Instruction_Cache", 16, 65, cacheLatency, requests};

    std::unordered_map<std::uint32_t, std::uint8_t> memRecord;

    void SetUp() override {
        for (auto& request: requests) {
            memRecord[request.addr + 0] = (request.data >> 0) & ((1 << 8) - 1);
            memRecord[request.addr + 1] = (request.data >> 8) & ((1 << 8) - 1);
            memRecord[request.addr + 2] = (request.data >> 16) & ((1 << 8) - 1);
            memRecord[request.addr + 3] = (request.data >> 24) & ((1 << 8) - 1);
        }
    }
};


TEST_P(IntegrationTests, DirectMapped) {
    Cache<MappingType::Direct> dataCache{"Data_cache",
                                         cacheLines, cacheLineSize, cacheLatency,
                                         nullptr};

    auto connections = connectComp(cpu, dataRam, instructionRam, dataCache, instructionCache);


    sc_start(2, SC_MS);
    ASSERT_EQ(cpu.results.size(), requests.size());

    for (int i = 0; i < requests.size(); ++i) {
        ASSERT_EQ(cpu.results.at(i).first, requests.at(i).addr);
        ASSERT_EQ(cpu.results.at(i).second, requests.at(i).data);
        ASSERT_EQ(memRecord[requests.at(i).addr], dataRam.dataMemory[requests.at(i).addr]);
        ASSERT_EQ(memRecord[requests.at(i).addr + 1], dataRam.dataMemory[requests.at(i).addr + 1]);
        ASSERT_EQ(memRecord[requests.at(i).addr + 2], dataRam.dataMemory[requests.at(i).addr + 2]);
        ASSERT_EQ(memRecord[requests.at(i).addr + 3], dataRam.dataMemory[requests.at(i).addr + 3]);
    }
};

TEST_P(IntegrationTests, FullyAssociative) {
    Cache<MappingType::Fully_Associative> dataCache{"Data_cache",
                                                    cacheLines, cacheLineSize, cacheLatency,
                                                    getReplacementPolity(policy, cacheLines)};

    auto connections = connectComp(cpu, dataRam, instructionRam, dataCache, instructionCache);
};

const auto memoryLatencyValues = testing::Values(0, 1, 10, 100, 100, 1<<31);
const auto cacheLatencyValues = testing::Values(0, 1, 10, 100, 100, 1<<31);
const auto cacheLines = testing::Values(0, 1, 5, 10, 100, 1<<31);
const auto cacheLineSizes = testing::Values(0, 16, 32, 64, 128, 1<<31);
const auto policies = testing::Values(POLICY_FIFO, POLICY_LRU, POLICY_RANDOM);
const auto numRequests = testing::Values(generateRandomRequests(0), generateRandomRequests(1), generateRandomRequests(10),
                                         generateRandomRequests(100), generateRandomRequests(1000),
                                         generateRandomRequests(10000), generateRandomRequests(1<<31));

INSTANTIATE_TEST_SUITE_P(AllCombinations, IntegrationTests,
                         testing::Combine(memoryLatencyValues, cacheLatencyValues, cacheLines, cacheLines,
                                          cacheLineSizes, policies));


std::vector<Request> generateRandomRequests(std::uint64_t len) {
    auto addresses = generateRandomVector(len, UINT32_MAX);
    auto data = generateRandomVector(len, UINT32_MAX);
    auto we = generateRandomVector(len, 1);

    std::vector<Request> requests;

    for (int i = 0; i < len; ++i) {
        requests.push_back(
                Request{
                        static_cast<std::uint32_t>(addresses.at(i)),
                        static_cast<std::uint32_t>(data.at(i)),
                        static_cast<int>(we.at(i))
                });
    }

    return requests;
}

std::unique_ptr<ReplacementPolicy<std::uint32_t>>
getReplacementPolity(CacheReplacementPolicy replacementPolicy, unsigned int cacheSize) {
    switch (replacementPolicy) {
        case POLICY_LRU:
            return std::make_unique<LRUPolicy<std::uint32_t>>(cacheSize);
        case POLICY_FIFO:
            return std::make_unique<FIFOPolicy<std::uint32_t>>(cacheSize);
        case POLICY_RANDOM:
            return std::make_unique<RandomPolicy<std::uint32_t>>(cacheSize);
        default:
            throw std::runtime_error("Encountered unknown policy type");
    }
}

template<MappingType mappingType>
std::unique_ptr<Connections> connectComp(CPU& cpu,
                                         RAM& dataRam, RAM& instructionRam,
                                         Cache<mappingType>& dataCache, InstructionCache& instructionCache) {
    auto connections = std::make_unique<Connections>();

    // Data Cache
    // CPU -> Cache
    cpu.weBus(connections->cpuWeSignal);
    cpu.addressBus(connections->cpuAddressSignal);
    cpu.dataOutBus(connections->cpuDataOutSignal);
    cpu.validDataRequestBus(connections->cpuValidDataRequestSignal);

    dataCache.cpuWeBus(connections->cpuWeSignal);
    dataCache.cpuAddrBus(connections->cpuAddressSignal);
    dataCache.cpuDataInBus(connections->cpuDataOutSignal);
    dataCache.cpuValidRequest(connections->cpuValidDataRequestSignal);

    // Cache -> CPU
    cpu.dataInBus(connections->cpuDataInSignal);
    cpu.dataReadyBus(connections->cpuDataReadySignal);

    dataCache.cpuDataOutBus(connections->cpuDataInSignal);
    dataCache.ready(connections->cpuDataReadySignal);

    // Cache -> RAM
    dataRam.weBus(connections->ramWeSignal);
    dataRam.addressBus(connections->ramAddressSignal);
    dataRam.validRequestBus(connections->ramValidRequestSignal);
    dataRam.dataInBus(connections->ramDataInSignal);

    dataCache.memoryWeBus(connections->ramWeSignal);
    dataCache.memoryAddrBus(connections->ramAddressSignal);
    dataCache.memoryValidRequestBus(connections->ramValidRequestSignal);
    dataCache.memoryDataOutBus(connections->ramDataInSignal);

    // RAM -> Cache
    dataRam.readyBus(connections->ramReadySignal);
    dataRam.dataOutBus(connections->ramDataOutSignal);

    dataCache.memoryReadyBus(connections->ramReadySignal);
    dataCache.memoryDataInBus(connections->ramDataOutSignal);

    // Instruction Cache
    // CPU -> Cache
    cpu.validInstrRequestBus(connections->validInstrRequestSignal);
    cpu.pcBus(connections->pcSignal);

    instructionCache.validInstrRequestBus(connections->validInstrRequestSignal);
    instructionCache.pcBus(connections->pcSignal);

    // Cache -> CPU
    cpu.instrBus(connections->instructionSignal);
    cpu.instrReadyBus(connections->instrReadySignal);

    instructionCache.instructionBus(connections->instructionSignal);
    instructionCache.instrReadyBus(connections->instrReadySignal);

    // Cache -> RAM
    instructionRam.addressBus(connections->instrRamAddressSignal);
    instructionRam.weBus(connections->instrRamWeBus);
    instructionRam.validRequestBus(connections->instrRamValidRequestBus);
    instructionRam.dataInBus(connections->instrRamDataInBus);

    instructionCache.memoryAddrBus(connections->instrRamAddressSignal);
    instructionCache.memoryWeBus(connections->instrRamWeBus);
    instructionCache.memoryValidRequestBus(connections->instrRamValidRequestBus);
    instructionCache.memoryDataOutBus(connections->instrRamDataInBus);

    // RAM -> Cache
    instructionRam.readyBus(connections->instrRamReadySignal);
    instructionRam.dataOutBus(connections->instrRamDataOutSignal);

    instructionCache.memoryReadyBus(connections->instrRamReadySignal);
    instructionCache.memoryDataInBus(connections->instrRamDataOutSignal);

    cpu.clock(connections->clk);
    dataCache.clock(connections->clk);
    instructionCache.clock(connections->clk);
    dataRam.clock(connections->clk);
    instructionRam.clock(connections->clk);

    return connections;
}

