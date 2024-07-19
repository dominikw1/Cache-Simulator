#pragma once

#include <cmath>
#include <cstdint>

inline void mergeSort(std::int64_t* array, std::int64_t from, std::int64_t to) {
    if (from >= to) {
        return;
    }

    std::int64_t len = to - from + 1;
    std::int64_t mid = (from + to) / 2;

    mergeSort(array, from, mid);
    mergeSort(array, mid + 1, to);

    std::int64_t mergeArray[len];
    std::int64_t leftArrayIndex = from;
    std::int64_t rightArrayIndex = mid + 1;

    for (std::int64_t i = 0; i < len; i++) {
        if (leftArrayIndex > mid) {
            mergeArray[i] = array[rightArrayIndex++];
        } else if (rightArrayIndex > to) {
            mergeArray[i] = array[leftArrayIndex++];
        } else if (array[leftArrayIndex] <= array[rightArrayIndex]) {
            mergeArray[i] = array[leftArrayIndex++];
        } else {
            mergeArray[i] = array[rightArrayIndex++];
        }
    }

    for (std::int64_t i = 0, j = from; i < len; i++, j++) {
        array[j] = mergeArray[i];
    }
}

inline std::int64_t getMaxPlaces(std::int64_t* array, std::int64_t len) {
    if (len == 0) {
        return 0;
    }

    std::int64_t max = 0;

    for (std::int64_t i = 0; i < len; ++i) {
        if (array[i] < 0 && array[i] * -1 > max) {
            max = array[i] * -1;
        } else if (array[i] > 0 && array[i] > max) {
            max = array[i];
        }
    }

    return (std::int64_t) log10(max) + 1;
}

inline void radixSort(std::int64_t* array, std::int64_t len) {
    std::int64_t max = getMaxPlaces(array, len);
    std::int64_t* posBuckets[10];
    std::int64_t* negBuckets[10];
    std::int64_t posBucketCount[10];
    std::int64_t negBucketCount[10];

    for (std::int64_t j = 0; j < 10; j++) {
        posBuckets[j] = (std::int64_t*) malloc(sizeof(std::int64_t) * len);
        negBuckets[j] = (std::int64_t*) malloc(sizeof(std::int64_t) * len);
    }

    for (std::int64_t i = 0; i < max; i++) {
        for (std::int64_t j = 0; j < 10; j++) {
            posBucketCount[j] = 0;
            negBucketCount[j] = 0;
        }

        for (std::int64_t j = 0; j < len; j++) {
            if (array[j] >= 0) {
                std::int64_t index = array[j] / ((std::int64_t) pow(10, i)) % 10;
                posBuckets[index][posBucketCount[index]++] = array[j];
            } else {
                std::int64_t index = (array[j] * -1) / ((std::int64_t) pow(10, i)) % 10;
                negBuckets[index][negBucketCount[index]++] = array[j];
            }
        }

        std::int64_t count = 0;
        for (std::int64_t j = 0; j < 10; ++j) {
            for (std::int64_t k = negBucketCount[j] - 1; k >= 0; --k) {
                array[count++] = negBuckets[j][k];
            }
        }
        for (std::int64_t j = 0; j < 10; ++j) {
            for (std::int64_t k = 0; k < posBucketCount[j]; ++k) {
                array[count++] = posBuckets[j][k];
            }
        }
    }

    for (std::int64_t j = 0; j < 10; j++) {
        free(posBuckets[j]);
        free(negBuckets[j]);
    }
}