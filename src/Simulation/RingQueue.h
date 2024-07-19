#pragma once
#include <mutex>
#include <vector>

template <typename T> class RingQueue {
    typedef typename std::array<T>::iterator ItType;
    std::vector<T> buffer;
    ItType begin, end;
    std::size_t currNumEl = 0;
    mutable std::mutex mutex;

  public:
    RingQueue(std::size_t size) : buffer{std::array<T>(size)}, begin{buffer.begin()}, end{buffer.begin()} {}

    std::size_t getSize() const {
        std::lock_guard<std::mutex> guard{mutex};
        return currNumEl;
    }
    std::size_t getCapacity() const {
        std::lock_guard<std::mutex> guard{mutex};
        return buffer.size();
    }
    bool isEmpty() const {
        std::lock_guard<std::mutex> guard{mutex};
        return currNumEl == 0;
    }

    void push(T el) {
        std::lock_guard<std::mutex> guard{mutex};
        assert(currNumEl < buffer.size());
        ++currNumEl;
        *end = el;
        ++end;
        if (end == buffer.end())
            end = buffer.begin();
    }

    T pop() {
        std::lock_guard<std::mutex> guard{mutex};
        assert(currNumEl > 0);
        T ret = *begin;
        ++begin;
        if (begin == buffer.end())
            begin = buffer.begin();
        --currNumEl;
        return ret;
    }

    template <typename PredicateType> bool any(PredicateType predicate) {
        auto curr = begin;
        std::size_t elChecked = 0;
        while (elChecked < currNumEl) {
            if (predicate(*curr)) {
                return true;
            }
            ++elChecked;
            ++curr;
            if (curr == buffer.end())
                curr = buffer.begin();
        }
        return false;
    }
};