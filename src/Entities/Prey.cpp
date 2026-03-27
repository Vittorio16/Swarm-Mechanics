#include <cmath>
#include <algorithm>
#include "Entities/Prey.h"
#include "Core/GlobalHelpers.h"

#include <iostream>

// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->speciesID = -1;
    this->maxSpeed = 35.0f;
    this->force = 600.0f;
    this->viewRadius = 20.0f;
    this->fovAngle = 360.0f;
    this->digestionTime = 0.5f;
    this->remainingDigestion = this->digestionTime;
}

// Updates the energy of the prey
void Prey::updateEnergy(float ax, float ay, float dt) {
    float actionEnergyCost = hypot(ax, ay) / this->force;
    float energyLoss = METABOLISM_COST * dt + actionEnergyCost * MAX_EFFORT_COST * dt;
    energy -= energyLoss;

    // Prey gain energy by eating grass -- handled in world.update
};

// Makes an agent reproduce, diminishing its energy 
// and creating a new agent with similar weights 
unique_ptr<Agent> Prey::reproduce(){
    float energyCost = MAX_ENERGY / 2.0f;
    energy -= energyCost * (1 + 0.3f * childCount);
    reproductionCooldown = 5.0f;
    
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