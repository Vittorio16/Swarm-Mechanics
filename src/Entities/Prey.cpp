#include <cmath>
#include <algorithm>
#include "Entities/Prey.h"
#include "Core/GlobalHelpers.h"

#include <iostream>

// Constructor
Prey::Prey(float x, float y): Agent(x, y) {
    this->speciesID = -1;
    this->maxSpeed =25.0f;
    this->force = 100.0f;
    this->viewRadius = 20.0f;
    this-> fovAngle = 120.0f;
}

// Helper: Safe grid access
float Prey::getFoodAt(int x, int y, const vector<vector<Cell>>& grid) {
    // Handle Wrapping
    if (x < 0) x += NUM_CELLE_X;
    else if (x >= NUM_CELLE_X) x -= NUM_CELLE_X;
    
    if (y < 0) y += NUM_CELLE_Y;
    else if (y >= NUM_CELLE_Y) y -= NUM_CELLE_Y;

    return grid[x][y].foodAmount;
}

// Senses food in an area around the agent, and returns its relative center of mass
vector<float> Prey::senseFood(const vector<vector<Cell>>& grid) {
    float maxFoodX = 0.0f;
    float maxFoodY = 0.0f;
    float maxFood = 0.0f;
    int radius = 5; //

    // Accumulate Global Vectors
    for (int i = -radius; i <= radius; i++) {
        for (int j = -radius; j <= radius; j++) {
            
            // Calculate actual grid coordinates with wrapping
            int cx = (int)x + i;
            int cy = (int)y + j;
            
            float food = getFoodAt(cx, cy, grid); 
            
            if (food > maxFood || (food == maxFood && (i*i + j*j < maxFoodX*maxFoodX + maxFoodY*maxFoodY))) {
                maxFoodX = i;
                maxFoodY = j;
                maxFood = food;
            }
        }
    }

    // Rotate to Local Space (Agent's Perspective)
    float heading = atan2(vy, vx);
    float c = cos(-heading);
    float s = sin(-heading);

    float localX = maxFoodX * c - maxFoodY * s;
    float localY = maxFoodX * s + maxFoodY * c;

    // Normalize inputs for the Brain
    float normalization = 10.0f; // Tune this based on typical food density

    return { 
        clamp(localX / normalization, -1.0f, 1.0f), 
        clamp(localY / normalization, -1.0f, 1.0f) 
    };
}

// Updates the prey's sensory data with info about smell
void Prey::updateSensoryData(const vector<Observation>& observations, const vector<float>& scentVals){
    Agent::updateSensoryData(observations, scentVals);

    sensors.foodSenseX = scentVals[0];
    sensors.foodSenseY= scentVals[1];
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
    energy -= 2 * energyCost ;

    float babyX = this->x + (randomFloat() * 10.0f - 5.0f);
    float babyY = this->y + (randomFloat() * 10.0f - 5.0f);

    auto baby = make_unique<Prey>(babyX, babyY);
    
    SimplePerceptron babyBrain = this->getBrain();
    babyBrain.mutate();

    baby->setBrain(babyBrain);
    baby->energy = energyCost;

    return baby;
}