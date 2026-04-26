#include <cmath>
#include <algorithm>
#include <cstdint>
#include "Entities/Agent.h"
#include "Core/GlobalHelpers.h"
#include "Core/Physics.h"

#include <iostream>

// Global counter for unique agent IDs
static uint64_t globalAgentIDCounter = 1;

// Helper function to find the closest enemy in the observation list
const Observation* Agent::getClosestEnemyObservation(const vector<Observation>& observations){
    const Observation* closestEnemy = nullptr;
    float minDistSq = INFINITY;
    
    // If any prey is within kill range, eat it (predators only)
    if (this->speciesID == PREDATOR_ID) {
        for (const auto& obs : observations) {
            if (obs.otherAgent->speciesID == PREY_ID && obs.distSq <= KILL_RANGE_SQ) {
                this->lockedTargetID = obs.otherAgent->id;
                return &obs;
            }
        }
    }

    // Don't give tunnel vision to prey
    if (this->lockedTargetID != 0 && this->id != PREY_ID) {
        for (const auto& obs : observations) {
            // Tunnel vision lock maintained
            if (obs.otherAgent->id == this->lockedTargetID) {
                return &obs; 
            }
        }
    }
 
    // Find the new closest enemy
    for (const auto& obs : observations){
        if (obs.otherAgent->speciesID != this->speciesID && obs.distSq < minDistSq){
            minDistSq = obs.distSq;
            closestEnemy = &obs;
        }
    }

    // Update the target lock for the next simulation tick
    if (closestEnemy != nullptr) {
        this->lockedTargetID = closestEnemy->otherAgent->id;
    } else {
        this->lockedTargetID = 0;
    }

    return closestEnemy;
}

// Constructor
Agent::Agent(float startX, float startY) : 
        timeLived(0), energyGained(0),
        x(startX), y(startY), vx(0), vy(0), speed(0), facingAngle(0), friction(FRICTION_COEFFICIENT), 
        energy(STARTING_ENERGY), childCount(0), reproductionCooldown(0),
        sensingRange(SENSING_RANGE),  grassViewRadius(GRASS_SENSING_RADIUS), isAlive(true),
        previousThrustIntent(0), previousTurnIntent(0),
        thrustIntent(0), turnIntent(0) {

        this->id = globalAgentIDCounter++;
        this->lockedTargetID = 0;
    }

// Transforms an array of observations (1 per visible agent) into sensory data processable by the brain
void Agent::updateSensoryData(const vector<Observation>& observations, const vector<float>& scents){
    sensors.agentSpeed = speed;
    sensors.closestEnemyX = 0;
    sensors.closestEnemyY = 0;
    sensors.closestEnemyDist = 1.0f;
    sensors.enemyClosingSpeed = 0;
    sensors.enemyTangentialSpeed = 0;
    sensors.closestEnemy = nullptr;
    sensors.energyReserve = 1 - this->energy / MAX_ENERGY;

    // Grass inputs  
    float normalization = this->grassViewRadius;
    float distToFood = hypot(scents[0], scents[1]);

    if (abs(scents[0]) < 0.001f && abs(scents[1]) < 0.001f) {
        sensors.foodClosingVelocity = 0.0f;
        sensors.foodTangentialVelocity = 0.0f;

        sensors.foodSenseX = 0.0f;
        sensors.foodSenseY = 0.0f;
        sensors.foodDistance = 1.0f; 
    } else {
        // Normalize the food sense vector, dividing the direction vector from the distance
        float normFoodX = scents[0] / distToFood;
        float normFoodY = scents[1] / distToFood;
        
        sensors.foodSenseX = normFoodX;
        sensors.foodSenseY = normFoodY;
    
        float foodDistance = distToFood / normalization;
        sensors.foodDistance = foodDistance;
    
        // Calculate the agent's heading for closing and tangential velocity calculations
        float c = cos(-facingAngle);
        float s = sin(-facingAngle);
    
        float vLongitudinal = vx * c - vy * s;
        float vTangential = vx * s + vy * c;
    
        float closingVelocity = vLongitudinal * normFoodX + vTangential * normFoodY;
        float tangentialVelocity = vLongitudinal * normFoodY - vTangential * normFoodX;
    
        sensors.foodClosingVelocity = closingVelocity / maxSpeed;
        sensors.foodTangentialVelocity = tangentialVelocity / maxSpeed;
    }

    // Terminate early if no enemies are visible
    if(observations.empty()) return;

    const Observation* closestEnemyObservation = getClosestEnemyObservation(observations);
    
    if (closestEnemyObservation != nullptr){
        float heading = facingAngle;
        float c = cos(-heading);
        float s = sin(-heading);
        
        float relx = closestEnemyObservation->dx;
        float rely = closestEnemyObservation->dy;
        
        float localX = relx * c - rely * s;
        float localY = relx * s + rely * c;
        
        float obsDist = closestEnemyObservation->dist;
        float closestEnemyX, closestEnemyY;

        if (obsDist <= 0.001f){
            closestEnemyX = 0;
            closestEnemyY = 0;
        } else {
            closestEnemyX = localX / obsDist;
            closestEnemyY = localY / obsDist;
        }
        
        sensors.closestEnemyX = closestEnemyX;
        sensors.closestEnemyY = closestEnemyY;
        sensors.closestEnemyDist = obsDist / viewRadius;

        float relvx = closestEnemyObservation->otherAgent->vx - this->vx;
        float relvy = closestEnemyObservation->otherAgent->vy - this->vy;

        float vLongitudinal = relvx * c - relvy * s;
        float vTangential = relvx * s + relvy * c;

        // Normalize 
        sensors.enemyClosingSpeed = vLongitudinal / closestEnemyObservation->otherAgent->maxSpeed;
        sensors.enemyTangentialSpeed = vTangential / closestEnemyObservation->otherAgent->maxSpeed;
        sensors.closestEnemy = closestEnemyObservation->otherAgent;
    } 
    return;
}

// Feeds the sensory data to the brain and updates thrust and turn intents
void Agent::think(){
    vector<float> neuralInputs = { 
        previousThrustIntent, previousTurnIntent,
        sensors.agentSpeed / maxSpeed, 
        sensors.closestEnemyX, sensors.closestEnemyY, 
        sensors.closestEnemyDist,
        sensors.enemyClosingSpeed, sensors.enemyTangentialSpeed,
        sensors.foodSenseX, sensors.foodSenseY, sensors.foodDistance,
        sensors.foodClosingVelocity, sensors.foodTangentialVelocity,
        sensors.energyReserve
    };

    vector<float> neuralOutput = brain.feedForward(neuralInputs);

    thrustIntent = max(0.0f, neuralOutput[0]);
    turnIntent = neuralOutput[1];

    previousThrustIntent = thrustIntent;
    previousTurnIntent = turnIntent;
}

// Updates the agent's position and velocity
void Agent::move(float dt){
    timeLived += dt;

    // Update heading and acceleration based on brain output
    facingAngle += turnIntent * MAXIMUM_TURNING_SPEED * dt;
    facingAngle = fmod(facingAngle, 2 * M_PI);
    if (facingAngle <= -M_PI) facingAngle += 2 * M_PI;
    if (facingAngle > M_PI) facingAngle -= 2 * M_PI;

    // Apply thrust in the direction of facingAngle
    float ax = thrustIntent * cos(facingAngle) * force;
    float ay = thrustIntent * sin(facingAngle) * force;

    // Update velocity using acceleration and friction
    vx += ax * dt;
    vy += ay * dt;
    
    float retention = max(0.0f, 1.0f - (friction * dt));
    vx *= retention;
    vy *= retention;

    // Checks constraint on speed
    speed = hypot(vx, vy);
    
    if (speed > maxSpeed){
        float excessRatio = maxSpeed / speed;
        vx *= excessRatio;
        vy *= excessRatio;
        speed = maxSpeed;
    }

    // Update position
    x += vx * dt;
    y += vy * dt;

    // Updates the agent's energy and checks reproduction
    if (remainingDigestion > 0.0f){
        remainingDigestion -= dt;
        if (remainingDigestion <= 0.001f) remainingDigestion = 0; 
    }
    if (reproductionCooldown > 0){
        reproductionCooldown -= dt;
        if (reproductionCooldown < 0) reproductionCooldown = 0;
    }

    updateEnergy(thrustIntent, turnIntent, dt);
    if (energy <= 0) isAlive = false;
    
}

// Sets the brain's weights like the given one
void Agent::setBrain(const SimplePerceptron& newBrain){
    this->brain = newBrain;
}