#include "../src/Simulation/CPU.h"
#include "../src/Simulation/Connections.h"
#include "../src/Simulation/Policy/LRUPolicy.h"
#include "../src/Simulation/Policy/Policy.h"
#include "../src/Simulation/Policy/RandomPolicy.h"

#include "Utils.h"
#include <gtest/gtest.h>
#include <systemc>

using namespace sc_core;
using namespace testing;

std::unique_ptr<ReplacementPolicy<std::uint32_t>> getReplacementPolicy(CacheReplacementPolicy policy,
                                                                       unsigned int cacheSize) {
    switch (policy) {
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

class IntegrationTests : public TestWithParam<std::tuple<unsigned int, unsigned int, unsigned int, unsigned int,
                                                         CacheReplacementPolicy, std::pair<Request*, int>>> {
  protected:
    unsigned int memoryLatency = std::get<0>(GetParam());
    unsigned int cacheLatency = std::get<1>(GetParam());
    unsigned int cacheLines = std::get<2>(GetParam());
    unsigned int cacheLineSize = std::get<3>(GetParam());
    CacheReplacementPolicy policy = std::get<4>(GetParam());

    Request* requests = std::get<5>(GetParam()).first;
    int requestsSize = std::get<5>(GetParam()).second;
    std::vector<Request> requestsAsVector = std::vector<Request>(requests, requests + requestsSize);

    CPU cpu{"CPU", requests};

    RAM dataRam{"Data_RAM", memoryLatency, cacheLineSize / 16};
    RAM instructionRam{"Instruction_RAM", memoryLatency, cacheLineSize / 16};

    InstructionCache instructionCache{"Instruction_Cache", 16, 64, cacheLatency, requestsAsVector};

    std::unordered_map<std::uint32_t, std::uint8_t> memRecord;
    std::unordered_map<std::uint32_t, std::uint32_t> readRecord;

    void SetUp() override {
        for (int i = 0; i < requestsSize; ++i) {
            if (requests[i].we) {
                memRecord[requests[i].addr + 0] = (requests[i].data >> 0) & ((1 << 8) - 1);
                memRecord[requests[i].addr + 1] = (requests[i].data >> 8) & ((1 << 8) - 1);
                memRecord[requests[i].addr + 2] = (requests[i].data >> 16) & ((1 << 8) - 1);
                memRecord[requests[i].addr + 3] = (requests[i].data >> 24) & ((1 << 8) - 1);
            } else {
                uint32_t expected = memRecord[requests[i].addr] | (memRecord[requests[i].addr + 1] << 8) |
                                    (memRecord[requests[i].addr + 2] << 16) | (memRecord[requests[i].addr + 3] << 24);
                readRecord[i] = expected;
            }
        }
    }
};

TEST_P(IntegrationTests, DirectMapped) {
    Cache<MappingType::Direct> dataCache{"Data_cache", cacheLines, cacheLineSize, cacheLatency};

    auto connections = connectComponents(cpu, dataRam, instructionRam, dataCache, instructionCache);

    sc_start(1, SC_MS);

    ASSERT_EQ(cpu.pcBus, requestsSize);

    for (int i = 0; i < requestsSize; ++i) {
        if (!requests[i].we) {
            ASSERT_EQ(requests[i].data, readRecord[i]);
        }
    }

    for (auto& mem : memRecord) {
        ASSERT_EQ(mem.second, dataRam.dataMemory[mem.first]);
    }
};

TEST_P(IntegrationTests, FullyAssociative) {
    Cache<MappingType::Fully_Associative> dataCache{"Data_cache", cacheLines, cacheLineSize, cacheLatency,
                                                    getReplacementPolicy(policy, cacheLines)};

    auto connections = connectComponents(cpu, dataRam, instructionRam, dataCache, instructionCache);

    sc_start(1, SC_MS);

    ASSERT_EQ(cpu.pcBus, requestsSize);

    for (int i = 0; i < requestsSize; ++i) {
        if (!requests[i].we) {
            ASSERT_EQ(requests[i].data, readRecord[i]);
        }
    }

    for (auto& mem : memRecord) {
        ASSERT_EQ(mem.second, dataRam.dataMemory[mem.first]);
    }
};

std::string ParamNameGenerator(const testing::TestParamInfo<IntegrationTests::ParamType>& info) {
    auto memoryLatency = std::to_string(std::get<0>(info.param));
    auto cacheLatency = std::to_string(std::get<1>(info.param));
    auto cacheLines = std::to_string(std::get<2>(info.param));
    auto cacheLineSize = std::to_string(std::get<3>(info.param));
    auto policy = std::to_string(std::get<4>(info.param));
    auto requests = std::to_string(std::get<5>(info.param).second);

    return memoryLatency + "_" + cacheLatency + "_" + cacheLines + "_" + cacheLineSize + "_" + policy + "_" + requests;
}

const auto memoryLatencies = testing::Values(0, 1, 10, 100);
const auto cacheLatencies = testing::Values(0, 1, 10, 100);
const auto cacheLines = testing::Values(1, 10, 100);
const auto cacheLineSizes = testing::Values(16, 32, 64, 128);
const auto policies = testing::Values(POLICY_FIFO, POLICY_LRU, POLICY_RANDOM);
const auto requests = testing::Values(std::pair<Request*, size_t>(generateRandomRequests(0), 0),
                                      std::pair<Request*, size_t>(generateRandomRequests(1), 1),
                                      std::pair<Request*, size_t>(generateRandomRequests(10), 10),
                                      std::pair<Request*, size_t>(generateRandomRequests(100), 100),
                                      std::pair<Request*, size_t>(generateRandomRequests(1000), 1000),
                                      std::pair<Request*, size_t>(generateRandomRequests(2000, 100), 2000));

INSTANTIATE_TEST_SUITE_P(AllCombinations, IntegrationTests,
                         Combine(memoryLatencies, cacheLatencies, cacheLines, cacheLineSizes, policies, requests),
                         ParamNameGenerator);
