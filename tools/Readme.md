# Tools

## MemoryAnalyser

MemoryAnalyser is a LLVM-Pass inserting calls to logging functions before every memory access. It writes its output to ``memory_analysis.csv``.

### Build

In the directory ``MemoryAnalyser`` simply run ``make build``. Make sure you have LLVM installed.

### Usage

If you want to compile the file ``example.cpp``, first include the header ``MemoryAnalyser/include/MemoryAnalyserSupport.h`` and then  run:

```
clang++ -fpass-plugin=MemoryAnalyser/build/MemoryAnalyser.so example.cpp
```

If you would like to run custom functions on every write and read, do not include the header and instead declare ``void logRead(void* address)`` and ``void logWrite(void* address, uint64_t value)`` anywhere in the global namespace.