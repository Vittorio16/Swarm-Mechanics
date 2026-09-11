#pragma once
#include <vector>
#include <cmath>
#include <cuda_runtime.h>
#include <math_constants.h>
using namespace std;

struct ThoroidalData {
    float dist;
    float distSq;
    float angleToTarget;
    float dx;
    float dy;
};

// Returns all the relevant data for sensory processing
inline __host__ __device__ ThoroidalData getThoroidalCoordinates(float obsX, float obsY, float targetX, float targetY, int worldWidth, int worldHeight){

    float dx = targetX - obsX;
    float dy = targetY - obsY;

    // Thoroidal Wrapping (Pac-Man Logic)
    if (dx > worldWidth * 0.5f) {
        dx -= worldWidth;
    } 
    else if (dx < -worldWidth * 0.5f) {
        dx += worldWidth;
    }
    if (dy > worldHeight * 0.5f) {
        dy -= worldHeight;
    } 
    else if (dy < -worldHeight * 0.5f) {
        dy += worldHeight;
    }

    float dist = hypotf(dx, dy);
    float distSq = dx*dx + dy*dy;
    float angleToTarget = atan2f(dy, dx);
    
    return {dist, distSq, angleToTarget, dx, dy};
}