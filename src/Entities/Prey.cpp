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
    this-> fovAngle = 120.0f;
}

// Helper: Safe grid access
float Prey::getFoodAt(int x, int y, const vector<vector<Cell>>& grid) {
    // Handle Wrapping
    if (isnan(x) || isinf(x)) x = 0.0f;
    x = fmod(x, NUM_CELLE_X);
    if (x < 0) x += NUM_CELLE_X;

    if (isnan(y) || isinf(y)) y = 0.0f;
    y = fmod(y, NUM_CELLE_Y);
    if (y < 0) y += NUM_CELLE_Y;
    
    return grid[x][y].foodAmount;
}

// Senses food in an area around the agent, and returns its relative center of mass
vector<float> Prey::senseFood(const vector<vector<Cell>>& grid) {
    float maxFoodCellX = 0.0f;
    float maxFoodCellY = 0.0f;
    
    float bestDistSq = 99999;
    float bestFoodScore = -1;

    int radius = 20;

    // Accumulate Global Vectors
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            
            if (i == 0 && j == 0) continue;

            // Calculate actual grid coordinates with wrapping
            int cx = (int)x + i;
            int cy = (int)y + j;
            
            float food = getFoodAt(cx, cy, grid); 
    
            if (food > 0){
                float distSq = (float)(i*i + j*j);
                float foodScore = food*food / distSq;

                if (foodScore > bestFoodScore) {
                    maxFoodCellX = i;
                    maxFoodCellY = j;

                    bestDistSq = distSq;
                    bestFoodScore = foodScore;
                }
            }
        }
    }

    if (bestFoodScore < 0) return {0, 0};

    // Rotate to Local Space (Agent's Perspective)
    float maxFoodX = maxFoodCellX + ((int)x - x) + 0.5f;
    float maxFoodY = maxFoodCellY + ((int)y - y) + 0.5f;
    
    float heading = facingAngle;
    float c = cos(-heading);
    float s = sin(-heading);

    float localX = maxFoodX * c - maxFoodY * s;
    float localY = maxFoodX * s + maxFoodY * c;

    return {localX, localY};
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
    float babyY = this->y + (randomFloat() * 2.0f - 2.0f);

    auto baby = make_unique<Prey>(babyX, babyY);
    
    SimplePerceptron babyBrain = this->getBrain();
    babyBrain.mutate();

    baby->setBrain(babyBrain);
    baby->energy = energyCost;

    return baby;
}