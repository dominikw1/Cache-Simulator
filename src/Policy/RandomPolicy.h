#pragma once
#include <cstdint>
#include <random>

template <typename T> class RandomPolicy : public ReplacementPolicy<T> {
  public:
    void logUse(T usage) override;
    T pop() override;
    std::size_t getSize() { return size; }
    RandomPolicy(std::size_t size) : size{size} {
        std::random_device randomDevice{}; // for simpler but less performant code we could just use this
        generator = std::mt19937{randomDevice()};
        randomDistr = std::uniform_int_distribution<>(0, size - 1);
    }

  private:
    const std::size_t size;
    std::uniform_int_distribution<> randomDistr;
    std::mt19937 generator;
};

template <typename T> void inline RandomPolicy<T>::logUse(__attribute__((unused)) T usage) {
    // no use to book-keep for random policy
}

// Precondition: FULL Cache
template <typename T> inline T RandomPolicy<T>::pop() { return randomDistr(generator); }
