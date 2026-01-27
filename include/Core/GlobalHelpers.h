#pragma once
#include <random>

inline float randomFloat(float min = -1.0f, float max = 1.0f) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}