#pragma once
#include <cstdint>

template <typename T> class ReplacementPolicy {
  public:
    virtual void logUse(T usage) = 0;
    virtual T pop() = 0;
    virtual std::size_t calcBasicGates() noexcept = 0;
};