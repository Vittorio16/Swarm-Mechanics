#include <cmath>
#include "Entities/Prey.h"

// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->speciesID = -1;
    this->maxSpeed = 50.0f;
    this->force = 200.0f;
    this->viewRadius = 30.0f;
    this-> fovAngle = 120.0f;
}

// Updates the energy of the prey
void Prey::updateEnergy(float speed, float ax, float ay, float dt) {
    float actionEnergyCost = hypot(ax, ay) / this->force;
    float energyLoss = METABOLISM_COST * dt + actionEnergyCost * MAX_EFFORT_COST * dt;
    energy -= energyLoss;

    // Prey right now gain energy by sgoing slower
    float energyGain = exp(-speed) * 10;
    energy += energyGain;
};