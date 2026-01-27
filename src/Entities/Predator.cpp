#include <cmath>
#include "Entities/Predator.h"
#include "Core/GlobalHelpers.h"

// Constructor
Predator::Predator(float x, float y): Agent(x, y) {
    this->speciesID = 1;
    this->maxSpeed = 30.0f;
    this->force = 75.0f;
    this->viewRadius = 30.0f;
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
        if (distSq < KILL_RANGE_SQ && sensors.closestEnemy->isAlive){
            energy += MAX_ENERGY / 2;

            sensors.closestEnemy->isAlive = false;
        }
    }
};

// Makes an agent reproduce, diminishing its energy 
// and creating a new agent with similar weights 
unique_ptr<Agent> Predator::reproduce(){
    float energyCost = MAX_ENERGY / 2.0f;
    energy -= energyCost;

    float babyX = this->x + (randomFloat() * 10.0f - 5.0f);
    float babyY = this->y + (randomFloat() * 10.0f - 5.0f);

    auto baby = make_unique<Predator>(babyX, babyY);
    
    SimplePerceptron babyBrain = this->getBrain();
    babyBrain.mutate();

    baby->setBrain(babyBrain);
    baby->energy = energyCost;
    return baby;
}