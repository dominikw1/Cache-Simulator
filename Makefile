# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

# entry point for the program and target name
C_SRCS = src/main.c 
CPP_SRCS = src/InstructionCache.cpp src/CacheInternal.cpp src/CPU.cpp src/Simulation.cpp

# Object files
C_OBJS = $(C_SRCS:.c=.o)
CPP_OBJS = $(CPP_SRCS:.cpp=.o)

# assignment task file
# HEADERS := src/CacheInternal.h src/CPU.h src/InternalRequests.h src/memory.hpp src/ReadOnlySpan.h src/Request.h src/Result.h src/Simulation.h 

# target name
TARGET := cache

# Path to your systemc installation
# TODO: Adapt to env variable
SCPATH = ../systemc

# Additional flags for the compiler
CXXFLAGS := -std=c++14  -I$(SCPATH)/include -L$(SCPATH)/lib -lsystemc -lm



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

test:
	cmake -S . -B build && cmake --build build && cd build && ctest --output-on-failure

.PHONY: all debug release clean