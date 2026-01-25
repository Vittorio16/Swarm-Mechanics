#include <cmath>
#include <algorithm>
#include "Entities/Agent.h"

// Constructor
Agent::Agent(float startX, float startY) : 
        x(startX), y(startY), vx(0), vy(0),
        ax(0), ay(0), friction(10.0f), isAlive(true) {}

// Transforms an array of observations (1 per visible agent) 
// into sensory data processable by the brain
// It returns the data about the closest agent of a different species
void Agent::updateSensoryData(const vector<Observation>& observations){
    if(observations.empty()){
        sensors.closestPredatorX = 0;
        sensors.closestPredatorY = 0;
        sensors.closestPredatorVx = 0;
        sensors.closestPredatorVy = 0;
    
        return;
    }
    const Observation* closestEnemy = nullptr;
    float minDistSq = INFINITY;
    
    for (const auto& obs : observations){
        if (obs.speciesID != this->speciesID && obs.distSq < minDistSq){
            minDistSq = obs.distSq;
            closestEnemy = &obs;
        }
    }
    if (closestEnemy != nullptr){
        sensors.closestPredatorX = closestEnemy->dx;
        sensors.closestPredatorY = closestEnemy->dy;
        sensors.closestPredatorVx = closestEnemy->vx;
        sensors.closestPredatorVy = closestEnemy->vy;
    }
    
    return;
}

void Agent::think(){
    vector<float> neualInputs = {sensors.closestPredatorX, sensors.closestPredatorY, 
                                sensors.closestPredatorVx, sensors.closestPredatorVy};
    
    vector<float> neuralOutput = brain.feedForward(neualInputs);

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
    float speed = hypot(vx, vy);

    if (speed > maxSpeed){
        float excessRatio = maxSpeed / speed;
        vx *= excessRatio;
        vy *= excessRatio;
    }

    // Update position
    x += vx * dt;
    y += vy * dt;

    // Reset acceleration for next frame
    ax = 0;
    ay = 0;
}