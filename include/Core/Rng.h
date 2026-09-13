#pragma once
#include <cuda_runtime.h>
#include <math_constants.h>
#include <cstdint>

__host__ __device__ __forceinline__
uint64_t splitmix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

// Separate streams per use
namespace RngPurpose {
    constexpr uint32_t AGENT_SPAWN  = 1;
    constexpr uint32_t BRAIN_INIT   = 2;
    constexpr uint32_t REPRODUCTION = 3;
    constexpr uint32_t MUTATION     = 4;
    constexpr uint32_t FOOD_SPAWN   = 5;
}

struct RngStream {
    uint64_t key;
    uint32_t counter;

    __host__ __device__ __forceinline__
    RngStream(uint64_t seed, uint64_t identity, uint64_t tick, uint32_t purpose) {
        key = splitmix64(seed
                       ^ splitmix64(identity)
                       ^ splitmix64(tick * 0x2545F4914F6CDD1DULL + purpose));
        counter = 0;
    }

    __host__ __device__ __forceinline__
    uint32_t nextBits() {
        return (uint32_t)(splitmix64(key + (uint64_t)(++counter)) >> 32);
    }

    __host__ __device__ __forceinline__
    float nextFloat() {
        return (float)(nextBits() >> 8) * (1.0f / 16777216.0f);
    }

    __host__ __device__ __forceinline__
    float nextFloat(float lo, float hi) { return lo + (hi - lo) * nextFloat(); }

    __host__ __device__ __forceinline__
    int nextInt(int n) { return (int)(nextBits() % (uint32_t)n); }

    // Standard normal, for Gaussian mutation
    __host__ __device__ __forceinline__
    float nextNormal() {
        float u1 = fmaxf(nextFloat(), 1e-7f);
        float u2 = nextFloat();
        return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * CUDART_PI_F * u2);
    }
};