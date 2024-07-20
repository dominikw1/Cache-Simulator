#pragma once
#include "../Saturating_Arithmetic.h"
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
    std::size_t getSize() const { return cache.size(); }
    LRUPolicy(std::size_t size) : size{size} {}
    constexpr std::size_t calcBasicGates() const noexcept override;

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

template <typename T> inline constexpr std::size_t LRUPolicy<T>::calcBasicGates() const noexcept {
    // this is difficult to say as we are really not emulating hardware very well here (lots of dynamic memory
    // allocation). This is not really necessary, but as there is no requirement of making this synthesisable, we opted
    // for a more general amortized O(1) approach
    // lets assume the linked list is an array of registers and we are storing about 32 bit in each T
    // further assume the hashings use the same hash table as used in the main Cache => 2753000
    return addSatUnsigned(mulSatUnsigned(static_cast<size_t>(32), getSize()), static_cast<size_t>(2753000));
}