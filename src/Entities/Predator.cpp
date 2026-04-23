#include <cmath>
#include "Entities/Predator.h"
#include "Core/GlobalHelpers.h"

// Constructor
Predator::Predator(float x, float y): Agent(x, y) {
    this->speciesID = PREDATOR_ID;
    this->maxSpeed = PREDATOR_MAX_SPEED;
    this->force = PREDATOR_FORCE;
    this->viewRadius = PREDATOR_VIEW_RADIUS;
    this->fovAngle = PREDATOR_FOV_ANGLE;
    this->digestionTime = PREDATOR_DIGESTION_TIME;
    this->remainingDigestion = this->digestionTime;
}

// Updates the energy of the predator
void Predator::updateEnergy(float ax, float ay, float dt) {
    float actionEnergyCost = hypot(ax, ay) / this->force;

    // Sharks -- spend more to stay alive, less to move
    float baseMetabolism = METABOLISM_COST * PREDATOR_METABOLISM_MULTIPLIER;
    float effortCost = actionEnergyCost * MAX_EFFORT_COST * PREDATOR_EFFORT_MULTIPLIER;

    float energyLoss = baseMetabolism * dt + effortCost * dt;
    energy -= energyLoss;

    // Predators gain energy by eating prey
    if (this->remainingDigestion > 0.001f) return;
    if (sensors.closestEnemy != nullptr && sensors.closestEnemy->isAlive){
        float distSq = sensors.closestEnemyX*sensors.closestEnemyX + sensors.closestEnemyY*sensors.closestEnemyY;
        if (distSq < KILL_RANGE_SQ && sensors.closestEnemy->isAlive){
            energy += PREDATOR_ENERGY_GAIN;
            energyGained += PREDATOR_ENERGY_GAIN;

            sensors.closestEnemy->isAlive = false;

            this->remainingDigestion = this->digestionTime;
        }
    }
    if (energy > 3 * MAX_ENERGY / 2) energy = 3 * MAX_ENERGY / 2;
};

// Makes an agent reproduce, diminishing its energy
// and creating a new agent with similar weights
unique_ptr<Agent> Predator::reproduce(){
    float energyCost = BASE_REPRODUCTION_COST;
    energy -= energyCost * (1 + REPRODUCTION_COST_SCALING * childCount);
    reproductionCooldown = REPRODUCTION_COOLDOWN;

    float babyX = this->x + (randomFloat() * 2.0f - 1.0f);
    float babyY = this->y + (randomFloat() * 2.0f - 1.0f);

    auto baby = make_unique<Predator>(babyX, babyY);
    
    SimplePerceptron babyBrain = this->getBrain();
    babyBrain.mutate();

    baby->setBrain(babyBrain);
    baby->energy = energyCost;

    this->childCount++;
    return baby;
}

float Predator::getFitness() const {
    float fitness = energyGained * PREDATOR_ENERGY_FITNESS_MULTIPLIER;

    if (isnan(fitness) || isinf(fitness)) return -1.0f;
    return fitness;
}