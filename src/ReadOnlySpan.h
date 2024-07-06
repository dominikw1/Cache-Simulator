#pragma once

#include <cstddef>
// inspired by https://stackoverflow.com/a/60151658

template <typename T> class ReadOnlySpan {
  private:
    const T* arrayPtr;
    std::size_t arraySize;

  public:
    ReadOnlySpan(const T* arr, std::size_t size) : arrayPtr{arr}, arraySize{size} {}
    const T& operator[](std::size_t i) const { return *arrayPtr[i]; }
    std::size_t size() const { return arraySize; }
    const T* begin() const { return arrayPtr; }
    const T* end() const { return arrayPtr + arraySize; }
    const T& operator++() { return *(++arrayPtr); }
    const T& top() const { return arrayPtr[0]; }
};