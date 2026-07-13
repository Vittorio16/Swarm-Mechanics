#pragma once
#include <vector>
#include "Learning/Perceptron.h"

using namespace std;

struct SwarmData{
    // Population Management
    int max_capacity;
    int current_count;

    // Agent Identification
    vector<int> ID;
    vector<int> speciesID;
    vector<bool> isAlive;

    // Fitness Metrics
    vector<float> timeLived;
    vector<float> energyGained;

    // Energy Metrics
    vector<float> energy;
    vector<float> digestionTime;
    vector<float> remainingDigestion;
    // Used to cap population
    vector<int> childCount;
    vector<float> reproductionCooldown;

    // Physics
    vector<float> friction;
    vector<float> force;
    vector<float> maxSpeed;
    vector<float> sensingRange;     // Senses enemies within this range
    vector<float> viewRadius;
    vector<float> fovAngle;
    vector<int> grassViewRadius;
    // Dybnamic physic variables
    vector<float> x, y;
    vector<float> vx, vy, speed;
    vector<float> facingAngle;

    // Sensory Data
    struct SensoryData {
        vector<int> lockedEnemyIndex, closestEnemyIndex;
        vector<float> closestEnemyX, closestEnemyY, closestEnemyDist;
        vector<float> enemyClosingSpeed, enemyTangentialSpeed;
        vector<float> foodSenseX, foodSenseY, foodDistance;
        vector<float> foodClosingVelocity, foodTangentialVelocity;
        vector<float> energyReserve;

        SensoryData(int capacity): 
            lockedEnemyIndex(capacity, 0),
            closestEnemyIndex(capacity, 0),
            closestEnemyX(capacity, 0.0f), 
            closestEnemyY(capacity, 0.0f), 
            closestEnemyDist(capacity, -1.0f),
            enemyClosingSpeed(capacity, 0.0f), 
            enemyTangentialSpeed(capacity, 0.0f),
            foodSenseX(capacity, 0.0f), 
            foodSenseY(capacity, 0.0f), 
            foodDistance(capacity, -1.0f),
            foodClosingVelocity(capacity, 0.0f), 
            foodTangentialVelocity(capacity, 0.0f),
            energyReserve(capacity, 0.0f) {}
    } sensors;


    // Neural Network Outputs
    vector<float> previousThrustIntent;
    vector<float> previousTurnIntent;

    vector<float> thrustIntent;
    vector<float> turnIntent;

    // Neural Network Brains
    struct BrainData{
        vector<float> w01;
        vector<float> w12;
        vector<float> b0;
        vector<float> b1;

        BrainData(int capacity): 
            w01(capacity * (INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE), 0.0f), 
            w12(capacity * (HIDDEN_LAYER_SIZE * OUTPUT_LAYER_SIZE), 0.0f), 
            b0(capacity * HIDDEN_LAYER_SIZE, 0.0f), 
            b1(capacity * OUTPUT_LAYER_SIZE, 0.0f) {}
    } brains;

    // Constructor
    SwarmData(int capacity): 
        max_capacity(capacity), 
        current_count(0),
        ID(capacity, 0),
        speciesID(capacity, 0),
        isAlive(capacity, false),
        timeLived(capacity, 0.0f),
        energyGained(capacity, 0.0f),
        energy(capacity, 0.0f),
        digestionTime(capacity, 0.0f),
        remainingDigestion(capacity, 0.0f),
        childCount(capacity, 0),
        reproductionCooldown(capacity, 0.0f),
        friction(capacity, 0.0f),
        force(capacity, 0.0f),
        maxSpeed(capacity, 0.0f),
        sensingRange(capacity, 0.0f),
        viewRadius(capacity, 0.0f),
        fovAngle(capacity, 0.0f),
        grassViewRadius(capacity, 0),
        x(capacity, 0.0f),
        y(capacity, 0.0f),
        vx(capacity, 0.0f),
        vy(capacity, 0.0f),
        speed(capacity, 0.0f),
        facingAngle(capacity, 0.0f),
        sensors(capacity),
        previousThrustIntent(capacity, 0.0f),
        previousTurnIntent(capacity, 0.0f),
        thrustIntent(capacity, 0.0f),
        turnIntent(capacity, 0.0f),
        brains(capacity) {}
};