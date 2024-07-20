#include "../MemoryAnalyser/include/MemoryAnalyserSupport.h"
#include "Sort.h"
#include <algorithm>
#include "FileReader.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        return -1;
    }

    char* filename = argv[1];
    int valueCount = atoi(argv[2]);

    std::vector<std::int64_t> values = readInValues(filename, valueCount);

    mergeSort(&(values[0]), 0, valueCount - 1);
    volatile long out =0;
    for (auto& v: values) {
       out = v;
    }
    return 0;
}
