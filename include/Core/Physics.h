#pragma once
#include <cmath>
#include <cuda_runtime.h>
#include <math_constants.h>
using namespace std;

// Cheap version, not using the atan2
struct ThoroidalDelta {
    float dx;
    float dy;
    float distSq;
};

struct ThoroidalData {
    float dist;
    float distSq;
    float angleToTarget;
    float dx;
    float dy;
};

// Cheap function to calculate distSq
__host__ __device__ __forceinline__ ThoroidalDelta thoroidalDelta(float obsX, float obsY, float targetX, float targetY, int worldWidth, int worldHeight){
    float dx = targetX - obsX;
    float dy = targetY - obsY;
     
    dx -= worldWidth * rintf(dx / worldWidth);
    dy -= worldHeight * rintf(dy / worldHeight);

    return {dx, dy, dx * dx + dy * dy};
}

// Returns all the relevant data for sensory processing - only called on enemies close enough
__host__ __device__ __forceinline__ ThoroidalData getThoroidalCoordinates(float obsX, float obsY, float targetX, float targetY, int worldWidth, int worldHeight){
    ThoroidalDelta d = thoroidalDelta(obsX, obsY, targetX, targetY, worldWidth, worldHeight);
    
    return {sqrtf(d.distSq), d.distSq, atan2f(d.dy, d.dx), d.dx, d.dy};
}