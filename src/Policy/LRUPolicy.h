#pragma once
#include "ReplacementPolicy.h"

#include <assert.h>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>

template <typename T> class LRUPolicy : public ReplacementPolicy<T> {
  public:
    void logUse(T usage) override;
    // Return by value since T is small
    T pop() override;
    std::size_t getSize() { return cache.size(); }
    LRUPolicy(std::size_t size) : size{size} {}
    constexpr std::size_t calcBasicGates() noexcept override;

  private:
    const std::size_t size;
    std::list<T> cache;
    // necessary because of dependant type
    typedef typename std::list<T>::iterator ItType;
    std::unordered_map<T, ItType> mapping;
};

// precondition: either usage is in the cache already OR there is enough space
// left
template <typename T> void inline LRUPolicy<T>::logUse(T usage) {
    if (mapping.count(usage) != 0) {
        const auto& it = mapping[usage];
        cache.erase(it);
        cache.push_front(usage);
    } else {
        assert(cache.size() < size);
        cache.push_front(usage);
    }
    mapping[usage] = cache.begin();
}

// Precondition: Non-Empty cache
template <typename T> inline T LRUPolicy<T>::pop() {
    assert(!cache.empty());
    const auto retVal = cache.back();
    cache.pop_back();
    assert(mapping.find(retVal) != mapping.end());
    mapping.erase(retVal);
    return retVal;
}

template <typename T> inline constexpr std::size_t LRUPolicy<T>::calcBasicGates() noexcept {
    // this is difficult to say as we are really not emulating hardware very well here (lots of dynamic memory
    // allocation). This is not really necessary, but as there is no requirement of making this synthesisable, we opted
    // for a more general amortized O(1) approach
    // lets assume the linked list is an array of registers and we are storing about 8 bit in each T
    // further assume the hashings are simple lookups of about again 8*n size
    // To try to more accurately represent how expensive this actually is this number will then be multiplied by 16
    // (fair dice roll (; )
    return 16 * (8 + 8) * getSize();
}