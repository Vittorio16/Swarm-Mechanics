#pragma once
#include <vector>

using namespace std;

struct ThoroidalData {
    float dist;
    float distSq;
    float angleToTarget;
    float dx;
    float dy;
};

ThoroidalData getThoroidalCoordinates(float x1, float y1, float x2, float y2, int worldWidth, int worldHeight);