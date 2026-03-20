#include <cmath>
#include "Entities/Predator.h"
#include "Core/GlobalHelpers.h"

// Constructor
Predator::Predator(float x, float y): Agent(x, y) {
    this->speciesID = 1;
    this->maxSpeed = 35.0f;
    this->force = 400.0f;
    this->viewRadius = 30.0f;
    this->fovAngle = 120.0f;
    this->digestionTime = 2.0f;
    this->remainingDigestion = this->digestionTime;
}

// Updates the energy of the predator
void Predator::updateEnergy(float ax, float ay, float dt) {
    float actionEnergyCost = hypot(ax, ay) / this->force;

    // Sharks -- spend more to stay alive, less to move
    float baseMetabolism = METABOLISM_COST * 5.0f;
    float effortCost = actionEnergyCost * MAX_EFFORT_COST * 0.2f;

    float energyLoss = baseMetabolism * dt + effortCost * dt;
    energy -= energyLoss;

    // Predators gain energy by eating prey
    if (this->remainingDigestion != 0) return;
    if (sensors.closestEnemy != nullptr && sensors.closestEnemy->isAlive){
        float distSq = sensors.closestEnemyX*sensors.closestEnemyX + sensors.closestEnemyY*sensors.closestEnemyY;
        if (distSq < KILL_RANGE_SQ && sensors.closestEnemy->isAlive){
            energy += MAX_ENERGY / 2;
            energyGained += MAX_ENERGY / 2;

            sensors.closestEnemy->isAlive = false;
        }
    }
    if (energy > 3 * MAX_ENERGY / 2) energy = 3 * MAX_ENERGY / 2;
};

// Makes an agent reproduce, diminishing its energy 
// and creating a new agent with similar weights 
unique_ptr<Agent> Predator::reproduce(){
    remainingDigestion = digestionTime;

    float energyCost = MAX_ENERGY / 2.0f;
    energy -= energyCost * 1.2f;

    float babyX = this->x + (randomFloat() * 2.0f - 1.0f);
    float babyY = this->y + (randomFloat() * 2.0f - 1.0f);

    auto baby = make_unique<Predator>(babyX, babyY);
    
    SimplePerceptron babyBrain = this->getBrain();
    babyBrain.mutate();

    baby->setBrain(babyBrain);
    baby->energy = energyCost;
    return baby;
}

float Predator::getFitness() const {
    float fitness = energyGained * 10.0f;

    if (isnan(fitness) || isinf(fitness)) return -1.0f;
    return fitness;
}