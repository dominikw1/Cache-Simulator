# ---------------------------------------
# CONFIGURATION BEGIN
# ---------------------------------------

MAIN := src/main.c
ASSIGNMENT := src/cache.hpp
TARGET := cache
SCPATH = ../systemc
CXXFLAGS := -std=c++14  -I$(SCPATH)/include -L$(SCPATH)/lib -lsystemc -lm


# ---------------------------------------
# CONFIGURATION END
# ---------------------------------------

# Determine if clang or gcc is available
CXX := $(shell command -v g++ || command -v clang++)
ifeq ($(strip $(CXX)),)
    $(error Neither clang++ nor g++ is available. Exiting.)
endif

# Add rpath except for MacOS
UNAME_S := $(shell uname -s)

ifneq ($(UNAME_S), Darwin)
    CXXFLAGS += -Wl,-rpath=$(SCPATH)/lib
endif


# Default to release build for both app and library
all: debug

# Debug build
debug: CXXFLAGS += -g
debug: $(TARGET)

# Release build
release: CXXFLAGS += -O2
release: $(TARGET)

# recipe for building the program
$(TARGET): $(MAIN) $(ASSIGNMENT)
	$(CXX) $(LDFLAGS) -o $@ $(MAIN) $(CXXFLAGS)

# clean up
clean:
	rm -f $(TARGET)

.PHONY: all debug release clean