#include <cmath>
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

// Private Helper: Get food at 1 specific pixel
float Prey::getFoodAt(float x, float y, const vector<vector<Cell>>& grid) {
    int ix = static_cast<int>(x);
    int iy = static_cast<int>(y);
    
    // Wrap coordinates (Pac-man)
    if (ix < 0) ix += NUM_CELLE_X;
    else if (ix >= NUM_CELLE_X) ix -= NUM_CELLE_X;
    
    if (iy < 0) iy += NUM_CELLE_Y;
    else if (iy >= NUM_CELLE_Y) iy -= NUM_CELLE_Y;

    // Safety check
    if (ix >= 0 && ix < NUM_CELLE_X && iy >= 0 && iy < NUM_CELLE_Y) {
        return grid[ix][iy].foodAmount / MAX_FOOD;
    }
    return 0.0f;
}

// Private Helper: Trace a line
float Prey::castFoodRay(float angle, float dist, const vector<vector<Cell>>& grid) {
    float totalSmell = 0.0f;
    int samples = 10;
    
    // Check points along the line
    for (int i = 1; i <= samples; i++) {
        float t = (float)i / samples;
        float currentDist = dist * t;

        float sampleX = this->x + cos(angle) * currentDist;
        float sampleY = this->y + sin(angle) * currentDist;

        totalSmell += getFoodAt(sampleX, sampleY, grid);
    }
    return totalSmell / samples;
}

// Called by update, get scents for a prey
vector<float> Prey::senseFood(const vector<vector<Cell>>& grid) {
    float heading = atan2(vy, vx);

    float halfFovRad = (this->fovAngle) * (M_PI / 360.0f);
    float angleOffset = halfFovRad * 0.75f;

    float center = castFoodRay(heading, viewRadius, grid);
    float left   = castFoodRay(heading - angleOffset, viewRadius, grid); // -30 deg
    float right  = castFoodRay(heading + angleOffset, viewRadius, grid); // +30 deg

    return {left, center, right};
}

// Updates the prey's sensory data with info about smell
void Prey::updateSensoryData(const vector<Observation>& observations, const vector<float>& scentVals){
    Agent::updateSensoryData(observations, scentVals);

    sensors.foodSenseLeft = scentVals[0];
    sensors.foodSenseLeft = scentVals[1];
    sensors.foodSenseLeft = scentVals[2];
}

// Updates the energy of the prey
void Prey::updateEnergy(float speed, float ax, float ay, float dt) {
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