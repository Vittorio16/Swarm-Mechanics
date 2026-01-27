#include <cmath>
#include "Entities/Predator.h"

// Constructor
Predator::Predator(float x, float y): Agent(x, y) {
    this->speciesID = 1;
    this->maxSpeed = 60.0f;
    this->force = 150.0f;
    this->viewRadius = 50.0f;
    this-> fovAngle = 90.0f;
}

// Updates the energy of the predator
void Predator::updateEnergy(float speed, float ax, float ay, float dt) {
    float actionEnergyCost = hypot(ax, ay) / this->force;
    float energyLoss = METABOLISM_COST * dt + actionEnergyCost * MAX_EFFORT_COST * dt;
    energy -= energyLoss;

    // Predators gain energy by eating prey
    if (sensors.closestEnemy != nullptr){
        float distSq = sensors.closestPredatorX*sensors.closestPredatorX + sensors.closestPredatorY*sensors.closestPredatorY;
        if (distSq < KILL_RANGE_SQ){
            energy += MAX_ENERGY / 2;

            sensors.closestEnemy->isAlive = false;
        }
    }
};