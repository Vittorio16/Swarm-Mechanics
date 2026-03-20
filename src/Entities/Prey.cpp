#include <cmath>
#include <algorithm>
#include "Entities/Prey.h"
#include "Core/GlobalHelpers.h"

#include <iostream>

// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->speciesID = -1;
    this->maxSpeed = 25.0f;
    this->force = 500.0f;
    this->viewRadius = 20.0f;
    this->eatRadius = 20;
    this-> fovAngle = 120.0f;
}

// Updates the prey's sensory data with info about smell
void Prey::updateSensoryData(const vector<Observation>& observations, const vector<float>& scentVals){
    Agent::updateSensoryData(observations, scentVals);

    // Normalize inputs for the Brain    
    float normalization = 10.0f;
    sensors.foodSenseX = clamp(scentVals[0] / normalization, -1.0f, 1.0f);
    sensors.foodSenseY = clamp(scentVals[1] / normalization, -1.0f, 1.0f);

    if (abs(sensors.foodSenseX) < 0.001f && abs(sensors.foodSenseY) < 0.001f) {
        sensors.foodClosingVelocity = 0.0f;
        sensors.foodTangentialVelocity = 0.0f;
        return;
    }

    float angleToFood = atan2(scentVals[1], scentVals[0]);
    float c = cos(-facingAngle);
    float s = sin(-facingAngle);

    float vLongitudinal = vx * c - vy * s;
    float vTangential = vx * s + vy * c;
    
    float ca = cos(angleToFood);
    float sa = sin(angleToFood);

    float closingVelocity = vLongitudinal * ca + vTangential * sa;
    float tangentialVelocity = -vLongitudinal * sa + vTangential * ca;

    sensors.foodClosingVelocity = closingVelocity / maxSpeed;
    sensors.foodTangentialVelocity = tangentialVelocity / maxSpeed;

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
    energy -= energyCost * 1.2f;

    float babyX = this->x + (randomFloat() * 2.0f - 1.0f);
    float babyY = this->y + (randomFloat() * 2.0f - 1.0f);

    auto baby = make_unique<Prey>(babyX, babyY);
    
    SimplePerceptron babyBrain = this->getBrain();
    babyBrain.mutate();

    baby->setBrain(babyBrain);
    baby->energy = energyCost;

    return baby;
}