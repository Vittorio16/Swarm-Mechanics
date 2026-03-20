#pragma once
#include <vector>

using namespace std;

struct ThoroidalData {
    float distSq;
    float angleToTarget;
    float dx;
    float dy;
};

ThoroidalData getThoroidalCoordinates(float x1, float y1, float x2, float y2, int worldWidth, int worldHeight);
// Returns a new heading, based on the wanted heading and turning factor
float lerpAngle(float current, float target, float factor);