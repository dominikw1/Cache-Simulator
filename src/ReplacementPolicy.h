#pragma once
template <typename T> class ReplacementPolicy {
  public:
    virtual void logUse(T usage) = 0;
    virtual T pop() = 0;
    //virtual ~ReplacementPolicy() = 0;
};