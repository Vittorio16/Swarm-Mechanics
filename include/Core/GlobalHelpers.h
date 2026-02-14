#pragma once
#include <random>

using namespace std;

float randomFloat(float min = -1.0f, float max = 1.0f);

// Returns a new heading, based on the wanted heading and turning factor
float lerpAngle(float current, float target, float factor);