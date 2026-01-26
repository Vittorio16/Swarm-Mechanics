#include <cmath>
#include "Entities/Prey.h"

// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->speciesID = -1;
    this->maxSpeed = 100.0f;
    this->force = 400.0f;
    this->viewRadius = 30.0f;
    this-> fovAngle = 120.0f;
}

// Updates the energy of the prey
void Prey::updateEnergy(float ax, float ay, float dt) {
    float actionEnergyCost = hypot(ax, ay) / this->force;
    float energyLoss = METABOLISM_COST * dt + actionEnergyCost * MAX_EFFORT_COST * dt;
    energy -= energyLoss;

    // Prey right now gain energy by standing still
    float energyGain = (ax < 5.0f && ay < 5.0f) ? MAX_ENERGY / 1000 : 0;
    energy += energyGain;
};