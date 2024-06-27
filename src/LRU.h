#pragma once
#include <assert.h>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <list>
#include <type_traits>
#include <unordered_map>

// TODO: consider constraining the type somehow. Maybe also reconsider making
// this a template
template <typename T> class LRU {
  public:
    void logUse(T usage);
    // Return by value since T is small
    T popLRU();
    std::size_t getSize() { return cache.size(); }
    LRU(std::size_t size): size{size} {}
  private:
    const std::size_t size;
    // TODO: consider writing custom (ARENA) memory allocator
    std::list<T> cache;
    // necessary because of dependant type
    typedef typename std::list<T>::iterator ItType;
    // TODO: consider making this an array
    std::unordered_map<T, ItType> mapping;
};

// precondition: either usage is in the cache already OR there is enough space
// left
template <typename T> void inline LRU<T>::logUse(T usage) {
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
template <typename T> inline T LRU<T>::popLRU() {
    assert(!cache.empty());
    const auto retVal = cache.back();
    cache.pop_back();
    assert(mapping.find(retVal) != mapping.end());
    mapping.erase(retVal);
    return retVal;
}
