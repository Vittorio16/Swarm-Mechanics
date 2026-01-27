#include <cmath>
#include "Entities/Prey.h"
#include "Core/GlobalHelpers.h"


// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->speciesID = -1;
    this->maxSpeed = 50.0f;
    this->force = 200.0f;
    this->viewRadius = 20.0f;
    this-> fovAngle = 120.0f;
}

// Updates the energy of the prey
void Prey::updateEnergy(float speed, float ax, float ay, float dt) {
    float actionEnergyCost = hypot(ax, ay) / this->force;
    float energyLoss = METABOLISM_COST * dt + actionEnergyCost * MAX_EFFORT_COST * dt;
    energy -= energyLoss;

    // Prey right now gain energy by sgoing slower
    float energyGain = exp(-speed) * 20;
    energy += energyGain;
};

// Makes an agent reproduce, diminishing its energy 
// and creating a new agent with similar weights 
unique_ptr<Agent> Prey::reproduce(){
    energy -= MAX_ENERGY / 2;

    float babyX = this->x + (randomFloat() * 10.0f - 5.0f);
    float babyY = this->y + (randomFloat() * 10.0f - 5.0f);

    auto baby = make_unique<Prey>(babyX, babyY);
    
    SimplePerceptron babyBrain = this->getBrain();
    babyBrain.mutate();

    baby->setBrain(babyBrain);

    return baby;
}