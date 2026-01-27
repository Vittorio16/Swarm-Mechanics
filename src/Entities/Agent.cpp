#include <cmath>
#include <algorithm>
#include "Entities/Agent.h"

// Helper function to find the closest enemy in the observation list
const Observation* Agent::getClosestEnemyObservation(const vector<Observation>& observations){
    const Observation* closestEnemy = nullptr;
    float minDistSq = INFINITY;
    
    for (const auto& obs : observations){
        if (obs.speciesID != this->speciesID && obs.distSq < minDistSq){
            minDistSq = obs.distSq;
            closestEnemy = &obs;
        }
    }

    return closestEnemy;
}
// Constructor
Agent::Agent(float startX, float startY) : 
        x(startX), y(startY), vx(0), vy(0),
        ax(0), ay(0), friction(2.0f), 
        energy(2 * MAX_ENERGY / 3), isAlive(true) {}

// Transforms an array of observations (1 per visible agent) 
// into sensory data processable by the brain
// It returns the data about the closest agent of a different species
void Agent::updateSensoryData(const vector<Observation>& observations){
    if(observations.empty()){
        sensors.closestPredatorX = 0;
        sensors.closestPredatorY = 0;
        sensors.closestPredatorVx = 0;
        sensors.closestPredatorVy = 0;
        sensors.closestEnemy = nullptr;

        return;
    }
    const Observation* closestEnemyObservation = getClosestEnemyObservation(observations);
    
    if (closestEnemyObservation != nullptr){
        sensors.closestPredatorX = closestEnemyObservation->dx;
        sensors.closestPredatorY = closestEnemyObservation->dy;
        sensors.closestPredatorVx = closestEnemyObservation->vx;
        sensors.closestPredatorVy = closestEnemyObservation->vy;
        sensors.closestEnemy = closestEnemyObservation->otherAgent;
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

    // Updates the agent's energy and checks reproduction
    updateEnergy(speed, ax, ay, dt);
    isAlive = energy <= 0 ? false : true;
    
    // Reproduction mechanism
    if (energy > MAX_ENERGY) reproduce();
    // Reset acceleration for next frame
    ax = 0;
    ay = 0;
}

// Makes an agent reproduce, diminishing its energy 
// and creating a new agent with similar weights 
void Agent::reproduce(){
    energy -= MAX_ENERGY / 2;
    
}