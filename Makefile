# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

# entry point for the program and target name
C_SRCS = src/main.c src/ArgParsing.c src/FileProcessor.c
CPP_SRCS = src/Simulation/SubRequest.cpp src/Simulation/Simulation.cpp src/Simulation/Cache.cpp

# Object files
C_OBJS = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)

# target name
TARGET := cache

# Path to systemc installation
SCPATH = $(SYSTEMC_HOME)

# Additional flags for the compiler
CXXFLAGS := -Wall -Wextra -pedantic -fsanitize=undefined -std=c++14  -I$(SCPATH)/include -L$(SCPATH)/lib -lsystemc -lm



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
	rm -rf src/*.o
	rm -f src/Simulation/*.o

test:
	cmake -S . -B build  && cmake --build build  -j16 && cd build && ctest --output-on-failure --parallel 16

buildWithRAMReadAfterWrite: CXXFLAGS += -g -DSTRICT_RAM_READ_AFTER_WRITES
buildWithRAMReadAfterWrite: $(TARGET)

buildWithStrictInstrOrder: CXXFLAGS += -g -DSTRICT_RAM_READ_AFTER_WRITES -DSTRICT_INSTRUCTION_ORDER
buildWithStrictInstrOrder: $(TARGET)

.PHONY: all debug release clean test buildWithStrictInstrOrder buildWithRAMReadAfterWrite