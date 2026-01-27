#pragma once
#include "Learning/Perceptron.h"

const float MAX_ENERGY = 100.0f;
const float METABOLISM_COST = 0.05f;
const float MAX_EFFORT_COST = 5.0f;
// Forward declaration of the world class
class World;
class Agent;

// Contains an observation of another agent from the POV of an observer
struct Observation{
    float dx, dy;
    float vx, vy;
    int speciesID;
    float distSq;
    Agent* otherAgent;

    Observation(float x, float y, float x_v, float y_v, int t, float dist, Agent* other) : 
                dx(x), dy(y), vx(x_v), vy(y_v), speciesID(t), distSq(dist), otherAgent(other) {}
};

class Agent{
    private:
    // The neural network which guides the agent's behaviour
    SimplePerceptron brain;

    protected:
    // Sensory inputs which guide decisions
    struct SensoryData {
        float closestPredatorX, closestPredatorY;
        float closestPredatorVx, closestPredatorVy;
        Agent* closestEnemy;
        
        SensoryData() : closestPredatorX(0), closestPredatorY(0), 
                        closestPredatorVx(0), closestPredatorVy(0), closestEnemy(nullptr) {}
    } sensors;
    // Physics variables
    float friction;
    float force;
    float maxSpeed;
    float ax, ay;
    // Energy determines the state of life of the agent
    float energy;

    public:
    float viewRadius;
    float fovAngle;
    // State variables
    int speciesID;
    bool isAlive;
    float x, y;
    float vx, vy;
    
    Agent(float startX, float startY);
    virtual ~Agent() = default;

    // Makes decisions based on observation
    void updateSensoryData(const vector<Observation>& observations);
    void think();

    // Updates the agent's state
    void move(float dt);
    virtual void updateEnergy(float speed, float ax, float ay, float dt) = 0;

    // Creates a new agent of the same species as the parent
    void reproduce();
    
    // Helper function to get the closes enemy from observations list
    const Observation* getClosestEnemyObservation(const vector<Observation>& observations);
};