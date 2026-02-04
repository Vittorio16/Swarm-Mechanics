#pragma once
#include <memory>
#include "Learning/Perceptron.h"

const float MAX_ENERGY = 100.0f;
const float METABOLISM_COST = 0.05f;
const float MAX_EFFORT_COST = 5.0f;
const float RANGE_OF_VISION_SQ = 16.0f;
const float DIGESTION_TIME = 4.0f;

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
        float agentSpeed;
        float closestEnemyX, closestEnemyY;
        float enemyClosingSpeed, enemyTangentialSpeed;
        float foodSenseX, foodSenseY;
        float foodClosingVelocity, foodTangentialVelocity;
        Agent* closestEnemy;
        
        SensoryData() : agentSpeed(0),
                        closestEnemyX(0), closestEnemyY(0), 
                        enemyClosingSpeed(0), enemyTangentialSpeed(0), closestEnemy(nullptr),
                        foodSenseX(0), foodSenseY(0), foodClosingVelocity(0), foodTangentialVelocity(0) {}
    } sensors;
    // Physics variables
    float friction;
    float force;
    float maxSpeed;
    float ax, ay;
    
    public:
    // Energy determines the state of life of the agent
    float energy;
    float remainingDigestion;

    float rangeOfVision;
    float viewRadius;
    float fovAngle;
    // State variables
    int speciesID;
    bool isAlive;
    float x, y;
    float vx, vy, speed;
    float facingAngle;
    
    Agent(float startX, float startY);
    virtual ~Agent() = default;

    // Makes decisions based on observation
    virtual void updateSensoryData(const vector<Observation>& observations, const vector<float>& scents);
    void think();

    // Updates the agent's state
    void move(float dt);
    virtual void updateEnergy(float ax, float ay, float dt) = 0;
    // Creates a new agent of the same species as the parent
    virtual unique_ptr<Agent> reproduce() = 0;

    // Helper function to get the closes enemy from observations list
    const Observation* getClosestEnemyObservation(const vector<Observation>& observations);

    // Helper function to get and set the brain of the reproducing agent
    const SimplePerceptron& getBrain() const {return brain;}
    void setBrain(const SimplePerceptron& babyBrain);
};