#ifndef RT_UTILS_CUDA_UTILS_H
#define RT_UTILS_CUDA_UTILS_H

#ifdef __CUDACC__
#include <cstdio>

#define CUDA_CHECK(call) do { \
    cudaError_t err = (call); \
    if (err != cudaSuccess) { \
        fprintf(stderr, "CUDA error at %s:%d: %s\n", __FILE__, __LINE__, cudaGetErrorString(err)); \
        return; \
    } \
} while(0)

#endif // __CUDACC__

#endif // RT_UTILS_CUDA_UTILS_H
