#include <cmath>
#include <algorithm>
#include "Entities/Agent.h"
#include "Core/GlobalHelpers.h"
#include "Core/Physics.h"

#include <iostream>

// Helper function to find the closest enemy in the observation list
const Observation* Agent::getClosestEnemyObservation(const vector<Observation>& observations){
    const Observation* closestEnemy = nullptr;
    float minDistSq = INFINITY;
    
    for (const auto& obs : observations){
        if (obs.otherAgent->speciesID != this->speciesID && obs.distSq < minDistSq){
            minDistSq = obs.distSq;
            closestEnemy = &obs;
        }
    }

    return closestEnemy;
}
// Constructor
Agent::Agent(float startX, float startY) : 
        timeLived(0), energyGained(0),
        x(startX), y(startY), vx(0), vy(0), facingAngle(0),
        ax(0), ay(0), friction(FRICTION_COEFFICIENT), 
        energy(STARTING_ENERGY), childCount(0), reproductionCooldown(0),
        sensingRange(SENSING_RANGE),  grassViewRadius(GRASS_SENSING_RADIUS), isAlive(true),
        previousThrustIntent(0), previousStrafeIntent(0) {}

// Transforms an array of observations (1 per visible agent) 
// into sensory data processable by the brain
// It returns the data about the closest agent of a different species
void Agent::updateSensoryData(const vector<Observation>& observations, const vector<float>& scents){
    sensors.agentSpeed = speed;
    sensors.closestEnemyX = 0;
    sensors.closestEnemyY = 0;
    sensors.closestEnemyDist = 1.0f;
    sensors.enemyClosingSpeed = 0;
    sensors.enemyTangentialSpeed = 0;
    sensors.closestEnemy = nullptr;
    sensors.fullness = this->remainingDigestion / this->digestionTime;

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

        // Normalize them here since I don't have access to enemy max speed in think
        sensors.enemyClosingSpeed = vLongitudinal / closestEnemyObservation->otherAgent->maxSpeed;
        sensors.enemyTangentialSpeed = vTangential / closestEnemyObservation->otherAgent->maxSpeed;
        sensors.closestEnemy = closestEnemyObservation->otherAgent;
    } 
    return;
}

void Agent::think(){
    vector<float> neuralInputs = { 
        previousThrustIntent, previousStrafeIntent,
        sensors.agentSpeed / maxSpeed, 
        sensors.closestEnemyX, sensors.closestEnemyY, 
        sensors.closestEnemyDist,
        sensors.enemyClosingSpeed, sensors.enemyTangentialSpeed,
        sensors.foodSenseX, sensors.foodSenseY, sensors.foodDistance,
        sensors.foodClosingVelocity, sensors.foodTangentialVelocity,
        sensors.fullness
    };

    vector<float> neuralOutput = brain.feedForward(neuralInputs);

    float thrust = force * neuralOutput[0];
    // Cap strafing to avoid orbiting
    float strafing = force * neuralOutput[1] * STRAFING_CAP;

    float heading = facingAngle;
    float c = cos(heading);
    float s = sin(heading);

    ax = thrust * c + strafing * s;
    ay = thrust * s - strafing * c;

    previousThrustIntent = neuralOutput[0];
    previousStrafeIntent = neuralOutput[1];
}

// Aggiorna la posizione dell'agente usando accelerazioni e dt
void Agent::move(float dt){
    timeLived += dt;

    // Update velocity using acceleration and friction
    vx += ax * dt;
    vy += ay * dt;
    
    // Ensures friction never reverses velocity
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

    // This makes for smooth turning, instead of instantaneous, and avoids crazy standstill rotation
    if (speed > MINIMUM_HEADING_UPDATE_SPEED){
        float targetAngle = atan2(vy, vx);
    
        float turnSpeed = MAXIMUM_TURNING_SPEED;
        // Ensures we never overshoot the wanted angle, no matted dt
        float factor = turnSpeed * dt;
        if (factor > 1) factor = 1;

        facingAngle = lerpAngle(facingAngle, targetAngle, factor);
    }
    // Ensure heading stays between -PI and PI
    facingAngle = fmod(facingAngle, 2 * M_PI);
    if (facingAngle <= -M_PI) facingAngle += 2 * M_PI;
    if (facingAngle > M_PI) facingAngle -= 2 * M_PI;

    // Update position
    x += vx * dt;
    y += vy * dt;

    // Updates the agent's energy and checks reproduction
    if (remainingDigestion != 0){
        remainingDigestion -= dt;
        if (remainingDigestion < 0) remainingDigestion = 0; 
    }
    if (reproductionCooldown > 0){
        reproductionCooldown -= dt;
        if (reproductionCooldown < 0) reproductionCooldown = 0;
    }

    updateEnergy(ax, ay, dt);
    if (energy <= 0) isAlive = false;
    
    // Reset acceleration for next frame
    ax = 0;
    ay = 0;
}

// Sets the brain's weights like the given one -- for newborns
void Agent::setBrain(const SimplePerceptron& newBrain){
    this->brain = newBrain;
}