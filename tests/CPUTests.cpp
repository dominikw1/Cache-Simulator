#include <gtest/gtest.h>

#include "../src/CPU.h"
#include "../src/Request.h"
#include "Utils.h"
#include <systemc>
#include <unordered_map>
#include <vector>
using namespace sc_core;

SC_MODULE(InstrMemoryMock) {
    std::vector<uint32_t> instructionsProvided;

    sc_in<bool> validInstrRequest;
    sc_in<std::uint32_t> pcBus;
    sc_out<Request> instrBus;
    sc_out<bool> instrReadyBus;

    std::unordered_map<std::uint32_t, Request> instructionMemory;
    void provideInstr() {
        if (validInstrRequest) {
            auto pc = pcBus.read();
            std::cout << "Providing instruction at " << pc << std::endl;
            if (instructionMemory.find(pc) == instructionMemory.end()) {
                std::cout << "Ending simulation" << std::endl;
                sc_stop();
                return;
            }
            auto requestToWrite = instructionMemory.at(pc);
            std::cout << "Located request to write" << std::endl;
            instrBus.write(requestToWrite);
            instrReadyBus.write(true);
            std::cout << "Recording instruction..." << std::endl;
            instructionsProvided.push_back(pc);
            std::cout << "Done providing instruction" << std::endl;
        }
    }

    void stopProviding() { instrReadyBus.write(false); }

    SC_CTOR(InstrMemoryMock) {
        SC_METHOD(provideInstr);
        sensitive << pcBus << validInstrRequest.pos();
        dont_initialize();

        SC_METHOD(stopProviding);
        sensitive << validInstrRequest.neg();
        dont_initialize();
    }
};

SC_MODULE(DataMemoryMock) {
    sc_in<bool> validDataRequest;
    sc_in<bool> weBus;
    sc_in<std::uint32_t> addressBus;
    sc_in<std::uint32_t> dataInBus;

    sc_out<std::uint32_t> dataOutBus;
    sc_out<bool> dataReadyBus;
    std::unordered_map<std::uint32_t, std::uint32_t> dataMemory;
    std::vector<Request> dataProvided;
    SC_CTOR(DataMemoryMock) {
        SC_METHOD(provideData);
        sensitive << dataInBus << addressBus << weBus << validDataRequest.pos();
        dont_initialize();

        SC_METHOD(stopProviding);
        sensitive << validDataRequest.neg();
        dont_initialize();
    }

    void stopProviding() { dataReadyBus.write(false); }

    void provideData() {
        if (validDataRequest) {
            // std::cout << "Providing/writing data at " << addressBus.read() << std::endl;
            if (weBus.read() == 0) {
                dataOutBus.write(dataMemory[addressBus.read()]);
            } else {
                dataMemory[addressBus.read()] = dataInBus.read();
            }
            dataReadyBus.write(true);
            dataProvided.push_back(Request{addressBus.read(), dataInBus.read(), weBus.read()});
            std::cout << "Done providing/writing data";
        }
    }
};

class CPUTests : public testing::Test {
  protected:
    CPU cpu{"cpu"};
    InstrMemoryMock instrMock{"instrMock"};
    DataMemoryMock dataMock{"dataMock"};
    sc_signal<bool> weSignal;
    sc_signal<std::uint32_t> addressSignal;
    sc_signal<std::uint32_t> dataOutSignal;
    sc_signal<bool, SC_MANY_WRITERS> validDataRequestSignal;

    // Cache -> CPU
    sc_signal<std::uint32_t> dataInSignal;
    sc_signal<bool, SC_MANY_WRITERS> dataReadySignal;

    // Instr Cache -> CPU
    sc_signal<Request> instrSignal;
    sc_signal<bool, SC_MANY_WRITERS> instrReadySignal;

    // CPU -> Instr Cache
    sc_signal<std::uint32_t> pcSignal;
    sc_signal<bool, SC_MANY_WRITERS> validInstrRequestSignal;

    sc_clock clock{"clk", sc_time(1, SC_NS)};
    void SetUp() {
        cpu.clock.bind(clock);
        cpu.weBus.bind(weSignal);
        cpu.addressBus.bind(addressSignal);
        cpu.dataOutBus.bind(dataOutSignal);

        cpu.dataInBus.bind(dataInSignal);
        cpu.dataReadyBus.bind(dataReadySignal);

        cpu.instrBus.bind(instrSignal);
        cpu.instrReadyBus.bind(instrReadySignal);

        cpu.pcBus.bind(pcSignal);
        cpu.validInstrRequestBus.bind(validInstrRequestSignal);
        cpu.validDataRequestBus.bind(validDataRequestSignal);

        std::cout << "done binding cpu" << std::endl;
        instrMock.pcBus.bind(pcSignal);
        instrMock.instrBus.bind(instrSignal);
        instrMock.instrReadyBus.bind(instrReadySignal);
        instrMock.validInstrRequest.bind(validInstrRequestSignal);

        dataMock.weBus.bind(weSignal);
        dataMock.addressBus.bind(addressSignal);
        dataMock.dataInBus.bind(dataOutSignal);
        dataMock.dataOutBus.bind(dataInSignal);
        dataMock.dataReadyBus.bind(dataReadySignal);
        dataMock.validDataRequest.bind(validDataRequestSignal);
        std::cout << "done binding mocks" << std::endl;
    }
};

TEST_F(CPUTests, CPUDispatchesSameInstructionAsInput) {
    auto req = Request{1, 5, 0};
    instrMock.instructionMemory[0] = req;
    dataMock.dataMemory[1] = 10;

    sc_start(1, SC_SEC);
    ASSERT_EQ(dataMock.dataProvided.size(), 1);
    ASSERT_EQ(dataMock.dataProvided.at(0), req);
}

TEST_F(CPUTests, CPUDispatchesSameMultipleInstructionsAsInput) {
    auto req1 = Request{1, 5, 0};
    auto req2 = Request{2, 5, 1};
    auto req3 = Request{5, 10, 1};

    instrMock.instructionMemory[0] = req1;
    instrMock.instructionMemory[1] = req2;
    instrMock.instructionMemory[2] = req3;

    dataMock.dataMemory[1] = 10;

    sc_start(1, SC_SEC);
    ASSERT_EQ(dataMock.dataProvided.size(), 3);
    ASSERT_EQ(dataMock.dataProvided.at(0), req1);
    ASSERT_EQ(dataMock.dataProvided.at(1), req2);
    ASSERT_EQ(dataMock.dataProvided.at(2), req3);
}

TEST_F(CPUTests, CPUWritesMemoryCorrectlySingleRequest) {
    auto reqW = Request{1, 5, 1};

    instrMock.instructionMemory[0] = reqW;

    sc_start(1, SC_SEC);
    ASSERT_EQ(dataMock.dataProvided.size(), 1);
    ASSERT_EQ(dataMock.dataProvided.at(0), reqW);

    ASSERT_EQ(dataMock.dataMemory[1], 5);
}

TEST_F(CPUTests, CPUWritesMemoryCorrectlyMultipleRequests) {
    auto reqW1 = Request{1, 5, 1};
    auto reqW2 = Request{10, 1, 1};
    auto reqW3 = Request{1100, 10, 1};

    instrMock.instructionMemory[0] = reqW1;
    instrMock.instructionMemory[1] = reqW2;
    instrMock.instructionMemory[2] = reqW3;

    sc_start(1, SC_SEC);
    ASSERT_EQ(dataMock.dataProvided.size(), 3);

    ASSERT_EQ(dataMock.dataMemory[1], 5);
    ASSERT_EQ(dataMock.dataMemory[10], 1);
    ASSERT_EQ(dataMock.dataMemory[1100], 10);
}

TEST_F(CPUTests, CPUHandlesRandomInputWithoutThrowing) {
    std::uint32_t numRequests = 100000;
    auto randomAddresses = generateRandomVector(numRequests, UINT32_MAX);
    auto randomData = generateRandomVector(numRequests, UINT32_MAX);
    auto randomWE = generateRandomVector(numRequests, 2);

    for (std::size_t i = 0; i < numRequests; ++i) {
        instrMock.instructionMemory[i] =
            Request{static_cast<std::uint32_t>(randomAddresses[i]), static_cast<std::uint32_t>(randomData[i]),
                    static_cast<int>(randomWE[i])};
    }

    sc_start(5, SC_SEC);
    ASSERT_EQ(dataMock.dataProvided.size(), numRequests);
}
