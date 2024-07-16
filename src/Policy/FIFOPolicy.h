#pragma once
#include "ReplacementPolicy.h"
#include "../RingQueue.h"
#include <unordered_set>

template <typename T> class FIFOPolicy : public ReplacementPolicy<T> {
  public:
    void logUse(T usage) override;
    T pop() override;
    std::size_t getSize() { return contents.getSize(); }
    FIFOPolicy(std::size_t size) : contents{size} {}

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
