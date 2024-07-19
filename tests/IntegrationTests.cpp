#include "../src/Simulation/CPU.h"
#include "../src/Simulation/InstructionCache.h"
#include "../src/Simulation/Memory.h"
#include "../src/Simulation/Policy/FIFOPolicy.h"
#include "../src/Simulation/Policy/LRUPolicy.h"
#include "../src/Simulation/Policy/Policy.h"
#include "../src/Simulation/Policy/RandomPolicy.h"
#include "../src/Simulation/Connections.h"

#include "Utils.h"
#include <gtest/gtest.h>
#include <systemc>

using namespace sc_core;

std::unique_ptr<ReplacementPolicy<std::uint32_t>>
getReplacementPolity(CacheReplacementPolicy replacementPolicy, unsigned int cacheSize);

class IntegrationTests
        : public ::testing::TestWithParam<std::tuple<unsigned int, unsigned int, unsigned int, unsigned int, CacheReplacementPolicy, std::vector<Request>>> {
protected:
    unsigned int memoryLatency = std::get<0>(GetParam());
    unsigned int cacheLatency = std::get<1>(GetParam());
    unsigned int cacheLines = std::get<2>(GetParam());
    unsigned int cacheLineSize = std::get<3>(GetParam());
    CacheReplacementPolicy policy = std::get<4>(GetParam());
    std::vector<Request> requests = std::get<5>(GetParam());

    CPU cpu{"CPU", requests.data(), requests.size()};

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
    Cache<MappingType::Direct> dataCache{"Data_cache", cacheLines, cacheLineSize, cacheLatency, nullptr};

    auto connections = connectComponents(cpu, dataRam, instructionRam, dataCache, instructionCache);

    sc_start(2, SC_MS);

    for (int i = 0; i < requests.size(); ++i) {
        if (requests.at(i).we) {
            ASSERT_EQ(memRecord[requests.at(i).addr], dataRam.dataMemory[requests.at(i).addr]);
            ASSERT_EQ(memRecord[requests.at(i).addr + 1], dataRam.dataMemory[requests.at(i).addr + 1]);
            ASSERT_EQ(memRecord[requests.at(i).addr + 2], dataRam.dataMemory[requests.at(i).addr + 2]);
            ASSERT_EQ(memRecord[requests.at(i).addr + 3], dataRam.dataMemory[requests.at(i).addr + 3]);
        } else {
            uint32_t expected = memRecord[requests.at(i).addr] | (memRecord[requests.at(i).addr + 1] << 8) |
                                (memRecord[requests.at(i).addr + 2] << 16) | (memRecord[requests.at(i).addr + 3] << 24);
            ASSERT_EQ(requests.at(i).data, expected);
        }
    }
};

TEST_P(IntegrationTests, FullyAssociative) {
    Cache<MappingType::Fully_Associative> dataCache{"Data_cache", cacheLines, cacheLineSize, cacheLatency,
                                                    getReplacementPolity(policy, cacheLines)};

    auto connections = connectComponents(cpu, dataRam, instructionRam, dataCache, instructionCache);
};

const auto memoryLatencyValues = testing::Values(0, 1, 10, 100, 1 << 31);
const auto cacheLatencyValues = testing::Values(0, 1, 10, 100, 1 << 31);
const auto cacheLines = testing::Values(1, 10, 100, 1 << 31);
const auto cacheLineSizes = testing::Values(16, 32, 64, 128, 1 << 31);
const auto policies = testing::Values(POLICY_FIFO, POLICY_LRU, POLICY_RANDOM);
const auto requests =
        testing::Values(generateRandomRequests(0), generateRandomRequests(1), generateRandomRequests(10),
                        generateRandomRequests(100), generateRandomRequests(1000), generateRandomRequests(1 << 20));

INSTANTIATE_TEST_SUITE_P(AllCombinations, IntegrationTests,
                         testing::Combine(memoryLatencyValues, cacheLatencyValues,
                                          cacheLines, cacheLineSizes,
                                          policies, requests));

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


