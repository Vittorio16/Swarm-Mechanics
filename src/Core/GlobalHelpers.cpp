#include <random>
#include <cmath>
#include "Core/GlobalHelpers.h"

float randomFloat(float min, float max) {
    thread_local static random_device rd;
    thread_local static mt19937 gen(rd());
    
    uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

// Returns a new heading, based on the wanted heading and turning factor
float lerpAngle(float current, float target, float factor) {
    float diff = target - current;
    
    // Handle wrapping (shortest path)
    if (isnan(diff) || isinf(diff)) diff = 0.0f;
    diff = fmod(diff, 2*M_PI);
    if (diff <= -M_PI) diff += 2 * M_PI;
    if (diff > M_PI) diff -= 2 * M_PI;
    
    return current + diff * factor;
}