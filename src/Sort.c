#include<stdlib.h>
#include<math.h>

void mergeSort(int* array, int from, int to) {
    if (from >= to) {
        return;
    }

    int len = to - from + 1;
    int mid = (from + to) / 2;

    mergeSort(array, from, mid);
    mergeSort(array, mid + 1, to);

    int mergeArray[len];
    int leftArrayIndex = from;
    int rightArrayIndex = mid + 1;

    for (int i = 0; i < len; i++) {
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

    for (int i = 0, j = from; i < len; i++, j++) {
        array[j] = mergeArray[i];
    }
}

int getMaxPlaces(int* array, int len) {
    if (len == 0) {
        return 0;
    }

    int max = 0;

    for (int i = 0; i < len; ++i) {
        if (array[i] < 0 && array[i] * -1 > max) {
            max = array[i] * -1;
        } else if (array[i] > 0 && array[i] > max) {
            max = array[i];
        }
    }

    return (int) log10(max) + 1;
}

void radixSort(int* array, int len) {
    int max = getMaxPlaces(array, len);
    int* posBuckets[10];
    int* negBuckets[10];
    int posBucketCount[10];
    int negBucketCount[10];

    for (int j = 0; j < 10; j++) {
        posBuckets[j] = malloc(sizeof(int) * len);
        negBuckets[j] = malloc(sizeof(int) * len);
    }

    for (int i = 0; i < max; i++) {
        for (int j = 0; j < 10; j++) {
            posBucketCount[j] = 0;
            negBucketCount[j] = 0;
        }

        for (int j = 0; j < len; j++) {
            if (array[j] >= 0) {
                int index = array[j] / ((int) pow(10, i)) % 10;
                posBuckets[index][posBucketCount[index]++] = array[j];
            } else {
                int index = (array[j] * -1) / ((int) pow(10, i)) % 10;
                negBuckets[index][negBucketCount[index]++] = array[j];
            }

        }

        int count = 0;
        for (int j = 0; j < 10; ++j) {
            for (int k = negBucketCount[j] - 1; k >= 0; --k) {
                array[count++] = negBuckets[j][k];
            }
        }
        for (int j = 0; j < 10; ++j) {
            for (int k = 0; k < posBucketCount[j]; ++k) {
                array[count++] = posBuckets[j][k];
            }
        }
    }
}