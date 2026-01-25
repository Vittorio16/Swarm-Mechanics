#include "Entities/Predator.h"

// Constructor
Predator::Predator(float x, float y): Agent(x, y) {
    this->speciesID = 1;
    this->maxSpeed = 120.0f;
    this->force = 250.0f;
    this->viewRadius = 75.0f;
    this-> fovAngle = 120.0f;
}