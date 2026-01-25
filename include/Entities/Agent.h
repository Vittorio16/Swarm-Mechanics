#pragma once
#include "Learning/Perceptron.h"

// Forward declaration of the world class
class World;

// Contains an observation of another agent from the POV of an observer
struct Observation{
    float dx, dy;
    float vx, vy;
    int type;
    float distSq;

    Observation(float x, float y, float x_v, float y_v, int t, float dist) : dx(x), dy(y), vx(x_v), vy(y_v), type(t), distSq(dist) {}

};

class Agent{
    private:
    // Sensory inputs which guide decisions
    struct SensoryData {
        float closestPredatorX, closestPredatorY;
        float closestPredatorVx, closestPredatorVy;

        SensoryData() : closestPredatorX(0), closestPredatorY(0), closestPredatorVx(0), closestPredatorVy(0) {}
    } sensors;

    // The neural network which guides the agent's behaviour
    SimplePerceptron brain;

    protected:
    // Physics variables
    float ax, ay;
    float force;
    float max_speed;
    float friction;

    public:
    // State variables
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
};