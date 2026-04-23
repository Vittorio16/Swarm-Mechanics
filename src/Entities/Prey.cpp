#include <cmath>
#include <algorithm>
#include "Entities/Prey.h"
#include "Core/GlobalHelpers.h"

#include <iostream>

// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->speciesID = PREY_ID;
    this->maxSpeed = PREY_MAX_SPEED;
    this->force = PREY_FORCE;
    this->viewRadius = PREY_VIEW_RADIUS;
    this->fovAngle = PREY_FOV_ANGLE;
    this->digestionTime = PREY_DIGESTION_TIME;
    this->remainingDigestion = this->digestionTime;
}

// Updates the energy of the prey
void Prey::updateEnergy(float ax, float ay, float dt) {
    float actionEnergyCost = hypot(ax, ay) / this->force;

    float metabolismCost = METABOLISM_COST;
    float effortCost = actionEnergyCost * MAX_EFFORT_COST * PREY_EFFORT_MULTIPLIER;

    float energyLoss = metabolismCost * dt + effortCost * dt;
    energy -= energyLoss;

    // Prey gain energy by eating grass -- handled in world.update
};

// Makes an agent reproduce, diminishing its energy 
// and creating a new agent with similar weights 
unique_ptr<Agent> Prey::reproduce(){
    float energyCost = PREY_ENERGY_GAIN;
    energy -= energyCost * (1 + REPRODUCTION_COST_SCALING * childCount);
    reproductionCooldown = REPRODUCTION_COOLDOWN;
    
    float babyX = this->x + (randomFloat() * 2.0f - 1.0f);
    float babyY = this->y + (randomFloat() * 2.0f - 1.0f);

    auto baby = make_unique<Prey>(babyX, babyY);
    
    SimplePerceptron babyBrain = this->getBrain();
    babyBrain.mutate();

    baby->setBrain(babyBrain);
    baby->energy = energyCost;

    this->childCount++;
    return baby;
}

// Gets the agent's fitness
float Prey::getFitness() const {
    float fitness = timeLived + (energyGained * PREY_ENERGY_FITNESS_MULTIPLIER);

    if (isnan(fitness) || isinf(fitness)) {
            return -1.0f; 
    }
    return fitness;
}