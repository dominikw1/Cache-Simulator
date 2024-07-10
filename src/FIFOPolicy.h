#pragma once
#include "ReplacementPolicy.h"
#include <unordered_set>
#include <vector>

template <typename T> class RingQueue {
    typedef typename std::vector<T>::iterator ItType;
    std::vector<T> buffer;
    ItType begin, end;
    std::size_t currNumEl = 0;

  public:
    RingQueue(std::size_t size) : buffer{std::vector<T>(size)}, begin{buffer.begin()}, end{buffer.begin()} {}

    std::size_t getSize() const { return currNumEl; }
    std::size_t getCapacity() const { return buffer.size(); }
    bool isEmpty() const { return currNumEl == 0; }

    void push(T el) {
        assert(currNumEl < buffer.size());
        ++currNumEl;
        *end = el;
        ++end;
        if (end == buffer.end())
            end = buffer.begin();
    }

    T pop() {
        assert(currNumEl > 0);
        T ret = *begin;
        ++begin;
        if (begin == buffer.end())
            begin = buffer.begin();
        --currNumEl;
        return ret;
    }
};

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
