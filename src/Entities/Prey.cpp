#include "Entities/Prey.h"

// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->speciesID = -1;
    this->maxSpeed = 100.0f;
    this->force = 400.0f;
    this->viewRadius = 50.0f;
    this-> fovAngle = 180.0f;
}