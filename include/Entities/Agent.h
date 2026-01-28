#pragma once
#include <memory>
#include "Learning/Perceptron.h"

const float MAX_ENERGY = 100.0f;
const float METABOLISM_COST = 0.05f;
const float MAX_EFFORT_COST = 5.0f;
const float RANGE_OF_VISION_SQ = 16.0f;

// Forward declaration of the world class
class World;
class Agent;

// Contains an observation of another agent from the POV of an observer
struct Observation{
    float dx, dy;
    float distSq;
    Agent* otherAgent;

    Observation(float x, float y, float dist, Agent* other) : 
                dx(x), dy(y), distSq(dist), otherAgent(other) {}
};

class Agent{
    private:
    // The neural network which guides the agent's behaviour
    SimplePerceptron brain;

    protected:
    // Sensory inputs which guide decisions
    struct SensoryData {
        float agentVx, agentVy;
        float closestPredatorX, closestPredatorY;
        float closestPredatorVx, closestPredatorVy;
        Agent* closestEnemy;
        
        SensoryData() : agentVx(0), agentVy(0),
                        closestPredatorX(0), closestPredatorY(0), 
                        closestPredatorVx(0), closestPredatorVy(0), closestEnemy(nullptr) {}
    } sensors;
    // Physics variables
    float friction;
    float force;
    float maxSpeed;
    float ax, ay;
    
    public:
    // Energy determines the state of life of the agent
    float energy;
    float rangeOfVision;
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
    virtual unique_ptr<Agent> reproduce() = 0;

    // Helper function to get the closes enemy from observations list
    const Observation* getClosestEnemyObservation(const vector<Observation>& observations);

    // Helper function to get and set the brain of the reproducing agent
    const SimplePerceptron& getBrain() const {return brain;}
    void setBrain(const SimplePerceptron& babyBrain);
};