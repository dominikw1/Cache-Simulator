#pragma once
#include "stdint.h"

struct Request {
    uint32_t addr;
    uint32_t data;
    int we;
};

// implementation of required methods for Request to be transmittable through systemc ports
#ifdef __cplusplus
#include <ostream>
#include <systemc>

static inline bool operator==(const Request& lhs, const Request& rhs) {
    return lhs.addr == rhs.addr && lhs.data == rhs.data && lhs.we == rhs.we;
}

static inline std::ostream& operator<<(std::ostream& os, const Request& request) {
    os << "Address: " << request.addr << "\n";
    os << "Data: " << request.data << "\n";
    os << "Write enabled: " << request.we << std::endl;
    return os;
}
static inline void sc_trace(sc_core::sc_trace_file* tf, const Request& request, const std::string& pre) {
    sc_core::sc_trace(tf, request.addr, pre + "_addr");
    sc_core::sc_trace(tf, request.data, pre + "_data");
    sc_core::sc_trace(tf, request.we, pre + "_we");
}
#endif
