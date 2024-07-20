#pragma once
#include "../RingQueue.h"
#include "ReplacementPolicy.h"

#include <unordered_set>

template <typename T> class FIFOPolicy : public ReplacementPolicy<T> {
  public:
    void logUse(T usage) override;
    T pop() override;
    std::size_t getSize() const { return contents.getSize(); }
    FIFOPolicy(std::size_t size) : contents{size} {}
    constexpr std::size_t calcBasicGates() const noexcept override;

  private:
    RingQueue<T> contents;
    std::unordered_set<T> itemsInCache;
};

template <typename T> inline void FIFOPolicy<T>::logUse(T usage) {
    if (itemsInCache.count(usage) == 0) {
        contents.push(usage);
        itemsInCache.insert(usage);
    }
}

template <typename T> inline T FIFOPolicy<T>::pop() {
    T popped = contents.pop();
    itemsInCache.erase(popped);
    return popped;
}

template <typename T> inline constexpr std::size_t FIFOPolicy<T>::calcBasicGates() const noexcept {
    // per register 4 gates and we have about 8* SIZE registers
    // for comparison whether already in we need SIZE comparators and then an or chain
    // an adder for incrementing
    return 10 * getSize() + 1;
}