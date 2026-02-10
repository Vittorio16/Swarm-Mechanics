#include <random>
#include "Core/GlobalHelpers.h"

float randomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

// Returns a new heading, based on the wanted heading and turning factor
float lerpAngle(float current, float target, float factor) {
    float diff = target - current;
    
    // Handle wrapping (shortest path)
    while (diff > M_PI) diff -= 2 * M_PI;
    while (diff < -M_PI) diff += 2 * M_PI;
    
    return current + diff * factor;
}