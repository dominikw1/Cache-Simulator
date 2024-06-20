#pragma once

#include <fstream>
#include <stdint.h>
#include <string>

const std::string filename = "memory_analysis.csv";

extern "C" void logRead(void* address) {
    std::ofstream ofs(filename, std::ios_base::app);
    ofs << "R;" << address << "\n";
    ofs.close();
}

extern "C" void logWrite(void* address, uint64_t value) {
    std::ofstream ofs(filename, std::ios_base::app);
    ofs << "W;" << address << ";" << value << "\n";
    ofs.close();
}