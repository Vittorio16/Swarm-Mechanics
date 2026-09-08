#pragma once
#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <thrust/device_ptr.h>
#include <thrust/fill.h>
#include "Core/Config.h"

struct SwarmData {
    // RNG generation for GPU
    struct RandData {
        curandState* state;
        void allocate(int capacity) {
            CUDA_CHECK(cudaMallocManaged(&state, capacity * sizeof(curandState)));
        }
        void free() {
            CUDA_CHECK(cudaFree(state));
        }
    } rng;

    // Population Management
    int max_capacity;
    int* current_count;

    // Agent Identification
    struct AgentIDData {
        uint64_t* ID;
        int* speciesID;
        // This needs to be int* otherwise it will not be aligned to 4, and thus not compatible with atomicExch
        int* isAlive;

        void allocate(int capacity){
            CUDA_CHECK(cudaMallocManaged(&ID, capacity * sizeof(uint64_t)));
            CUDA_CHECK(cudaMallocManaged(&speciesID, capacity * sizeof(int)));
            CUDA_CHECK(cudaMallocManaged(&isAlive, capacity * sizeof(int)));

            CUDA_CHECK(cudaMemset(ID, 0, capacity * sizeof(uint64_t)));
            CUDA_CHECK(cudaMemset(speciesID, 0, capacity * sizeof(int)));
            CUDA_CHECK(cudaMemset(isAlive, 0, capacity * sizeof(int)));
        }

        void free(){
            CUDA_CHECK(cudaFree(ID));
            CUDA_CHECK(cudaFree(speciesID));
            CUDA_CHECK(cudaFree(isAlive));
        }
    } agentIdentifications;

    // Fitness Metrics
    struct FitnessData {
        float* timeLived;
        float* energyGained;

        void allocate(int capacity){
            CUDA_CHECK(cudaMallocManaged(&timeLived, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&energyGained, capacity * sizeof(float)));

            CUDA_CHECK(cudaMemset(timeLived, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(energyGained, 0, capacity * sizeof(float)));
        }

        void free(){
            CUDA_CHECK(cudaFree(timeLived));
            CUDA_CHECK(cudaFree(energyGained));
        }
    } fitnessMetrics;

    // Energy Metrics
    struct EnergyData {
        float* energy;
        float* digestionTime;
        float* remainingDigestion;
        int* childCount;
        float* reproductionCooldown;

        void allocate(int capacity){
            CUDA_CHECK(cudaMallocManaged(&energy, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&digestionTime, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&remainingDigestion, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&childCount, capacity * sizeof(int)));
            CUDA_CHECK(cudaMallocManaged(&reproductionCooldown, capacity * sizeof(float)));

            CUDA_CHECK(cudaMemset(energy, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(digestionTime, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(remainingDigestion, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(childCount, 0, capacity * sizeof(int)));
            CUDA_CHECK(cudaMemset(reproductionCooldown, 0, capacity * sizeof(float)));
        }

        void free(){
            CUDA_CHECK(cudaFree(energy));
            CUDA_CHECK(cudaFree(digestionTime));
            CUDA_CHECK(cudaFree(remainingDigestion));
            CUDA_CHECK(cudaFree(childCount));
            CUDA_CHECK(cudaFree(reproductionCooldown));
        }
    } energyMetrics;

    // Physics
    struct PhysicsData {
        float *friction, *force, *maxSpeed;
        float *x, *y, *vx, *vy, *speed, *facingAngle;

        void allocate(int capacity) {
            CUDA_CHECK(cudaMallocManaged(&friction, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&force, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&maxSpeed, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&x, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&y, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&vx, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&vy, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&speed, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&facingAngle, capacity * sizeof(float)));

            CUDA_CHECK(cudaMemset(friction, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(force, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(maxSpeed, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(x, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(y, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(vx, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(vy, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(speed, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(facingAngle, 0, capacity * sizeof(float)));
        }

        void free() {
            CUDA_CHECK(cudaFree(friction));
            CUDA_CHECK(cudaFree(force));
            CUDA_CHECK(cudaFree(maxSpeed));
            CUDA_CHECK(cudaFree(x));
            CUDA_CHECK(cudaFree(y));
            CUDA_CHECK(cudaFree(vx));
            CUDA_CHECK(cudaFree(vy));
            CUDA_CHECK(cudaFree(speed));
            CUDA_CHECK(cudaFree(facingAngle));
        }
    } physics;

    // Perception Metrics
    struct PerceptionData {
        float* sensingRange;     
        float* viewRadius;
        float* fovAngle;
        float* grassViewRadius;

        void allocate(int capacity){
            CUDA_CHECK(cudaMallocManaged(&sensingRange, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&viewRadius, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&fovAngle, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&grassViewRadius, capacity * sizeof(float)));

            CUDA_CHECK(cudaMemset(sensingRange, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(viewRadius, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(fovAngle, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(grassViewRadius, 0, capacity * sizeof(float)));
        }

        void free(){
            CUDA_CHECK(cudaFree(sensingRange));
            CUDA_CHECK(cudaFree(viewRadius));
            CUDA_CHECK(cudaFree(fovAngle));
            CUDA_CHECK(cudaFree(grassViewRadius));
        }
    } perceptions;

    // Sensory Data
    struct SensoryData {
        int *lockedEnemyIndex, *closestEnemyIndex;
        float *closestEnemyX, *closestEnemyY, *closestEnemyDist;
        float *enemyClosingSpeed, *enemyTangentialSpeed;
        float *foodSenseX, *foodSenseY, *foodDistance;
        float *foodClosingVelocity, *foodTangentialVelocity;
        float *energyReserve;

        void allocate(int capacity){
            CUDA_CHECK(cudaMallocManaged(&lockedEnemyIndex, capacity * sizeof(int)));
            CUDA_CHECK(cudaMallocManaged(&closestEnemyIndex, capacity * sizeof(int)));
            CUDA_CHECK(cudaMallocManaged(&closestEnemyX, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&closestEnemyY, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&closestEnemyDist, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&enemyClosingSpeed, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&enemyTangentialSpeed, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&foodSenseX, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&foodSenseY, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&foodDistance, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&foodClosingVelocity, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&foodTangentialVelocity, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&energyReserve, capacity * sizeof(float)));

            CUDA_CHECK(cudaMemset(lockedEnemyIndex, 0, capacity * sizeof(int)));
            CUDA_CHECK(cudaMemset(closestEnemyIndex, 0, capacity * sizeof(int)));
            CUDA_CHECK(cudaMemset(closestEnemyX, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(closestEnemyY, 0, capacity * sizeof(float)));
            thrust::device_ptr<float> enemy_ptr(closestEnemyDist);
            thrust::fill(enemy_ptr, enemy_ptr + capacity, -1);
            CUDA_CHECK(cudaMemset(enemyClosingSpeed, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(enemyTangentialSpeed, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(foodSenseX, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(foodSenseY, 0, capacity * sizeof(float)));
            thrust::device_ptr<float> food_ptr(foodDistance);
            thrust::fill(food_ptr, food_ptr + capacity, -1);
            CUDA_CHECK(cudaMemset(foodClosingVelocity, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(foodTangentialVelocity, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(energyReserve, 0, capacity * sizeof(float)));
        }

        void free(){
            CUDA_CHECK(cudaFree(lockedEnemyIndex));
            CUDA_CHECK(cudaFree(closestEnemyIndex));
            CUDA_CHECK(cudaFree(closestEnemyX));
            CUDA_CHECK(cudaFree(closestEnemyY));
            CUDA_CHECK(cudaFree(closestEnemyDist));
            CUDA_CHECK(cudaFree(enemyClosingSpeed));
            CUDA_CHECK(cudaFree(enemyTangentialSpeed));
            CUDA_CHECK(cudaFree(foodSenseX));
            CUDA_CHECK(cudaFree(foodSenseY));
            CUDA_CHECK(cudaFree(foodDistance));
            CUDA_CHECK(cudaFree(foodClosingVelocity));
            CUDA_CHECK(cudaFree(foodTangentialVelocity));
            CUDA_CHECK(cudaFree(energyReserve));
        }
    } sensors;

    // Neural Network Outputs
    struct NeuralOutputData {
        float* thrustIntent;
        float* turnIntent;
        float* previousThrustIntent;
        float* previousTurnIntent;

        void allocate(int capacity){
            CUDA_CHECK(cudaMallocManaged(&thrustIntent, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&turnIntent, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&previousThrustIntent, capacity * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&previousTurnIntent, capacity * sizeof(float)));

            CUDA_CHECK(cudaMemset(thrustIntent, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(turnIntent, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(previousThrustIntent, 0, capacity * sizeof(float)));
            CUDA_CHECK(cudaMemset(previousTurnIntent, 0, capacity * sizeof(float)));
        }

        void free(){
            CUDA_CHECK(cudaFree(thrustIntent));
            CUDA_CHECK(cudaFree(turnIntent));
            CUDA_CHECK(cudaFree(previousThrustIntent));
            CUDA_CHECK(cudaFree(previousTurnIntent));
        }
    } neuralOutputs;

    // Neural Network Brains
    struct BrainData{
        float* w01;
        float* w12;
        float* b0;
        float* b1;

        void allocate(int capacity){
            int w01_elements = capacity * (INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE);
            int w12_elements = capacity * (HIDDEN_LAYER_SIZE * OUTPUT_LAYER_SIZE);
            int b0_elements = capacity * HIDDEN_LAYER_SIZE;
            int b1_elements = capacity * OUTPUT_LAYER_SIZE;

            CUDA_CHECK(cudaMallocManaged(&w01, w01_elements * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&w12, w12_elements * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&b0, b0_elements * sizeof(float)));
            CUDA_CHECK(cudaMallocManaged(&b1, b1_elements * sizeof(float)));

            CUDA_CHECK(cudaMemset(w01, 0, w01_elements * sizeof(float)));
            CUDA_CHECK(cudaMemset(w12, 0, w12_elements * sizeof(float)));
            CUDA_CHECK(cudaMemset(b0, 0, b0_elements * sizeof(float)));
            CUDA_CHECK(cudaMemset(b1, 0, b1_elements * sizeof(float)));
        }

        void free(){
            CUDA_CHECK(cudaFree(w01));
            CUDA_CHECK(cudaFree(w12));
            CUDA_CHECK(cudaFree(b0));
            CUDA_CHECK(cudaFree(b1));
        }
    } brains;

    // Constructor orchestrates allocations
    SwarmData(int capacity) : max_capacity(capacity) {
        CUDA_CHECK(cudaMallocManaged(&current_count, sizeof(int)));
        *current_count = 0;

        rng.allocate(capacity);
        agentIdentifications.allocate(capacity);
        fitnessMetrics.allocate(capacity);
        energyMetrics.allocate(capacity);
        physics.allocate(capacity);
        perceptions.allocate(capacity);
        sensors.allocate(capacity);
        neuralOutputs.allocate(capacity);
        brains.allocate(capacity);
    }

    // Destructor cleanly releases memory - cannot actually use destructor, since CPU
    // would call it while GPU is still copying the structure
    void freeAll() {
        CUDA_CHECK(cudaFree(current_count));
        rng.free();
        agentIdentifications.free();
        fitnessMetrics.free();
        energyMetrics.free();
        physics.free();
        perceptions.free();
        sensors.free();
        neuralOutputs.free();
        brains.free();
    }
};