#include "Entities/Predator.h"

// Constructor
Predator::Predator(float x, float y): Agent(x, y) {
    this->max_speed = 120.0f;
    this->force = 250.0f;
}