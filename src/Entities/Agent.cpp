#include <cmath>
#include "Entities/Agent.h"
#include <iostream>

// Constructor
Agent::Agent(float startX, float startY) : 
        x(startX), y(startY), vx(0), vy(0),
        ax(0), ay(0), friction(10.0f), isAlive(true) {}

// Transforms an array of observations (1 per visible agent) 
// into sensory data processable by the brain
void Agent::updateSensoryData(const vector<Observation>& observations){

}

void Agent::think(){
    vector<float> neualInputs = {sensors.closestPredatorX, sensors.closestPredatorY, 
                                sensors.closestPredatorVx, sensors.closestPredatorVy};
    
    vector<float> neuralOutput = brain.feedForward(neualInputs);
    cout << "ax: " << ax << ", ay: " << ay << endl;
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

    if (speed > max_speed){
        float excessRatio = max_speed / speed;
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