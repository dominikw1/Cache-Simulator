#include "../MemoryAnalyser/include/MemoryAnalyserSupport.h"
#include "Sort.h"

int main() {
    auto values = readInValues("input.txt");
    mergeSort(&(values[0]), 0, values.size());
    for (auto& v : values) {
        printf("%ld", v); // to ensure compiler cant optimise sort away
    }
    return 0;
}
