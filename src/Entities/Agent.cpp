#include <cmath>
#include <algorithm>
#include "Entities/Agent.h"

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
        x(startX), y(startY), vx(0), vy(0),
        ax(0), ay(0), friction(1.0f), 
        energy(2 * MAX_ENERGY / 3), rangeOfVision(RANGE_OF_VISION_SQ), isAlive(true) {}

// Transforms an array of observations (1 per visible agent) 
// into sensory data processable by the brain
// It returns the data about the closest agent of a different species
void Agent::updateSensoryData(const vector<Observation>& observations, const vector<float>& scents){
    sensors.agentSpeed = speed;
    sensors.closestEnemyX = 0;
    sensors.closestEnemyY = 0;
    sensors.enemyClosingSpeed = 0;
    sensors.enemyTangentialSpeed = 0;
    sensors.closestEnemy = nullptr;
    
    if(observations.empty()) return;

    const Observation* closestEnemyObservation = getClosestEnemyObservation(observations);
    
    if (closestEnemyObservation != nullptr){
        float heading = atan2(vy, vx);
        float c = cos(-heading);
        float s = sin(-heading);

        float relx = closestEnemyObservation->dx;
        float rely = closestEnemyObservation->dy;

        float localX = relx * c - rely * s;
        float localY = relx * s + rely * c;

        sensors.closestEnemyX = localX;
        sensors.closestEnemyY = localY;

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
    if (speciesID == -1 && (sensors.foodSenseX != 0 || sensors.foodSenseY != 0)) cout << "x: " << sensors.foodSenseX << ", y: " << sensors.foodSenseY << endl;
    vector<float> neuralInputs = { 
        sensors.agentSpeed / maxSpeed, 
        sensors.closestEnemyX / viewRadius, sensors.closestEnemyY / viewRadius, 
        sensors.enemyClosingSpeed, sensors.enemyTangentialSpeed,
        sensors.foodSenseX, sensors.foodSenseY
    };

    vector<float> neuralOutput = brain.feedForward(neuralInputs);
    ax = force * neuralOutput[0];
    ay = force * neuralOutput[1];
}

// Aggiorna la posizione dell'agente usando accelerazioni e dt
void Agent::move(float dt){
    // Update velocity using acceleration and friction
    vx += ax * dt;
    vy += ay * dt;

    vx -= vx * friction * dt;
    vy -= vy * friction * dt;

    // Checks constraint on speed
    speed = hypot(vx, vy);

    if (speed > maxSpeed){
        float excessRatio = maxSpeed / speed;
        vx *= excessRatio;
        vy *= excessRatio;
    }
    // Update position
    x += vx * dt;
    y += vy * dt;

    // Updates the agent's energy and checks reproduction
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