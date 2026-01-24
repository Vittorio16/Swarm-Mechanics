#include "Entities/Prey.h"

// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->max_speed = 100.0f;
    this->force = 400.0f;
}