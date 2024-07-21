C_SRCS = src/main.c src/ArgParsing.c src/FileProcessor.c
CPP_SRCS = src/Simulation/SubRequest.cpp src/Simulation/Simulation.cpp src/Simulation/Cache.cpp src/Simulation/CPU.cpp

C_OBJS = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)

TARGET := cache

SCPATH = $(SYSTEMC_HOME)

CFLAGS := -Wall -Wextra -pedantic  -std=c17
CXXFLAGS := -Wall -Wextra -pedantic  -std=c++14  -I$(SCPATH)/include -L$(SCPATH)/lib -lsystemc -lm



CXX := $(shell command -v g++ || command -v clang++)
ifeq ($(strip $(CXX)),)
    $(error Neither clang++ nor g++ is available. Exiting.)
endif

CC := $(shell command -v gcc || command -v clang)
ifeq ($(strip $(CC)),)
    $(error Neither clang nor gcc is available. Exiting.)
endif


UNAME_S := $(shell uname -s)

ifneq ($(UNAME_S), Darwin)
    CXXFLAGS += -Wl,-rpath=$(SCPATH)/lib
endif


all: debug

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

debug: CXXFLAGS += -g
debug: $(TARGET)

release: CXXFLAGS += -O2
release: $(TARGET)

$(TARGET): $(C_OBJS) $(CPP_OBJS)
	$(CXX) $(CXXFLAGS) $(C_OBJS) $(CPP_OBJS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET)
	rm -f src/*.o
	rm -f src/Simulation/*.o

test:
	cmake -S . -B build
	cmake --build build  -j16
	cd build; ctest --output-on-failure --parallel 16

integration:
	cmake -DBUILD_INTEGRATION_TESTING=ON -S . -B build
	cmake --build build  -j16
	cd build; ctest --output-on-failure --parallel 16

benchmarks:
	cd tools; make; python3 BenchmarkRunner.py

buildWithRAMReadAfterWrite: CXXFLAGS += -DSTRICT_RAM_READ_AFTER_WRITES
buildWithRAMReadAfterWrite: $(TARGET)

buildWithStrictInstrOrder: CXXFLAGS += -DSTRICT_RAM_READ_AFTER_WRITES -DSTRICT_INSTRUCTION_ORDER
buildWithStrictInstrOrder: $(TARGET)


.PHONY: all debug release clean test buildWithStrictInstrOrder buildWithRAMReadAfterWrite benchmarks
