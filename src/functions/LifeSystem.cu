#include "Core/GlobalHelpers.h"
#include "Core/Physics.h"
#include "functions/LifeSystem.h"
#include "functions/BrainSystem.h"
#include <cuda_runtime.h>

namespace LifeSystem {
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_int_distribution<int> disX(0, NUM_CELLE_X - 1);
    thread_local std::uniform_int_distribution<int> disY(0, NUM_CELLE_Y - 1);
}

__global__ void setupCurandKernel(curandState* state, unsigned long seed, int max_capacity) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= max_capacity) return;

    curand_init(seed, i, 0, &state[i]);
}

__global__ void createAgentsKernel(SwarmData swarm, int num_prey, int num_predators){
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= *swarm.current_count) return;

    int species = (i < num_prey) ? PREY_ID : PREDATOR_ID;

    curandState localState = swarm.rng.state[i];

    // Shift the world_id 48 bits to the left, leaving 48 bits for the local counter
    uint64_t world_prefix = static_cast<uint64_t>(swarm.world_id) << 48;
    swarm.agentIdentifications.ID[i] = world_prefix | static_cast<uint64_t>(i);

    swarm.agentIdentifications.speciesID[i] = species;
    swarm.agentIdentifications.isAlive[i] = 1;

    swarm.physics.x[i] = curand_uniform(&localState) * (float)NUM_CELLE_X;
    swarm.physics.y[i] = curand_uniform(&localState) * (float)NUM_CELLE_Y;
    swarm.physics.facingAngle[i] = (curand_uniform(&localState) * 2.0f - 1.0f) * CUDART_PI_F;
    swarm.physics.vx[i] = 0.0f;
    swarm.physics.vy[i] = 0.0f;
    swarm.physics.speed[i] = 0.0f;
    swarm.physics.friction[i] = FRICTION_COEFFICIENT;

    swarm.energyMetrics.energy[i] = STARTING_ENERGY;
    swarm.energyMetrics.digestionTime[i] = (species == PREY_ID) ? PREY_DIGESTION_TIME : PREDATOR_DIGESTION_TIME;
    swarm.energyMetrics.remainingDigestion[i] = 0.0f;
    swarm.energyMetrics.reproductionCooldown[i] = 0.0f;
    swarm.energyMetrics.childCount[i] = 0;

    swarm.physics.maxSpeed[i] = (species == PREY_ID) ? PREY_MAX_SPEED : PREDATOR_MAX_SPEED;
    swarm.physics.force[i] = (species == PREY_ID) ? PREY_FORCE : PREDATOR_FORCE;

    swarm.perceptions.viewRadius[i] = (species == PREY_ID) ? PREY_VIEW_RADIUS : PREDATOR_VIEW_RADIUS;
    swarm.perceptions.fovAngle[i] = (species == PREY_ID) ? PREY_FOV_ANGLE : PREDATOR_FOV_ANGLE;
    swarm.perceptions.sensingRange[i] = SENSING_RANGE;
    swarm.perceptions.grassViewRadius[i] = GRASS_SENSING_RADIUS;

    swarm.fitnessMetrics.timeLived[i] = 0.0f;
    swarm.fitnessMetrics.energyGained[i] = 0.0f;

    swarm.rng.state[i] = localState;
}

void LifeSystem::initSwarm(SwarmData& swarm, int num_prey, int num_predators){
    int grid_size = (MAX_SWARM_CAPACITY + BLOCK_SIZE - 1) / BLOCK_SIZE;
    setupCurandKernel<<<grid_size, BLOCK_SIZE>>>(swarm.rng.state, std::random_device{}(), MAX_SWARM_CAPACITY);

    *swarm.current_count = num_prey + num_predators;

    int i_grid_size = (*swarm.current_count + BLOCK_SIZE - 1) / BLOCK_SIZE;
    createAgentsKernel<<<i_grid_size, BLOCK_SIZE>>>(swarm, num_prey, num_predators);

    *swarm.agentIdentifications.localAgentIDCounter = *swarm.current_count;
    
    // Initializes random brains for the whole swarm
    BrainSystem::initRandom(swarm);
}

// Count survivors and write dead agents to the graveyard
__global__ void evaluateAndCountAliveKernel(SwarmData swarm, GraveyardData graveyard, int active_agents) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= active_agents) return;

    if (swarm.agentIdentifications.isAlive[i]) {
        atomicAdd(swarm.compaction.alive_count, 1);
    } else {
        // Calculate Fitness for dead agent
        float fitness;
        if (swarm.agentIdentifications.speciesID[i] == PREY_ID) {
            fitness = swarm.fitnessMetrics.timeLived[i] + (swarm.fitnessMetrics.energyGained[i] * PREY_ENERGY_FITNESS_MULTIPLIER);
            if (swarm.energyMetrics.energy[i] > 0) fitness *= PREY_HUNTED_PENALTY;
        } else {
            fitness = swarm.fitnessMetrics.energyGained[i];
        }

        // Write to Graveyard
        if (fitness >= MINIMUM_FITNESS_TO_BE_SAVED) {
            int slot = atomicAdd(graveyard.current_count, 1);
            if (slot < graveyard.max_capacity) {
                graveyard.speciesID[slot] = swarm.agentIdentifications.speciesID[i];
                graveyard.fitness[slot] = fitness;
                
                for (int j = 0; j < W01_SIZE; j++) graveyard.w01[slot * W01_SIZE + j] = swarm.brains.w01[i * W01_SIZE + j];
                for (int j = 0; j < W12_SIZE; j++) graveyard.w12[slot * W12_SIZE + j] = swarm.brains.w12[i * W12_SIZE + j];
                for (int j = 0; j < B0_SIZE; j++) graveyard.b0[slot * B0_SIZE + j] = swarm.brains.b0[i * B0_SIZE + j];
                for (int j = 0; j < B1_SIZE; j++) graveyard.b1[slot * B1_SIZE + j] = swarm.brains.b1[i * B1_SIZE + j];
            } else {
                atomicSub(graveyard.current_count, 1); // Graveyard full
            }
        }
    }
}

// Map the Holes and Movers based on the boundary 
__global__ void mapCompactionKernel(SwarmData swarm, int active_agents) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= active_agents) return;

    int boundary = *swarm.compaction.alive_count;
    // This always bocks because if there are n alive agents they will fit in the first n slots
    if (i < boundary && !swarm.agentIdentifications.isAlive[i]) {
        int idx = atomicAdd(swarm.compaction.hole_count, 1);
        swarm.compaction.holes_array[idx] = i;
    }
    if (i >= boundary && swarm.agentIdentifications.isAlive[i]) {
        int idx = atomicAdd(swarm.compaction.mover_count, 1);
        swarm.compaction.movers_array[idx] = i;
    }
}

// Pair holes and movers up and relocate the data
__global__ void moveCompactionKernel(SwarmData swarm, int active_agents) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    
    // Only launch as many threads as there are holes
    if (i >= *swarm.compaction.hole_count) return;

    int dest = swarm.compaction.holes_array[i];
    int src = swarm.compaction.movers_array[i];

    // Transfer all SoA data from src to dest
    swarm.rng.state[dest] = swarm.rng.state[src];
    
    swarm.agentIdentifications.ID[dest] = swarm.agentIdentifications.ID[src];
    swarm.agentIdentifications.speciesID[dest] = swarm.agentIdentifications.speciesID[src];
    swarm.agentIdentifications.isAlive[dest] = swarm.agentIdentifications.isAlive[src];
    
    swarm.fitnessMetrics.timeLived[dest] = swarm.fitnessMetrics.timeLived[src];
    swarm.fitnessMetrics.energyGained[dest] = swarm.fitnessMetrics.energyGained[src];
    
    swarm.energyMetrics.energy[dest] = swarm.energyMetrics.energy[src];
    swarm.energyMetrics.digestionTime[dest] = swarm.energyMetrics.digestionTime[src];
    swarm.energyMetrics.remainingDigestion[dest] = swarm.energyMetrics.remainingDigestion[src];
    swarm.energyMetrics.childCount[dest] = swarm.energyMetrics.childCount[src];
    swarm.energyMetrics.reproductionCooldown[dest] = swarm.energyMetrics.reproductionCooldown[src];
    
    swarm.physics.friction[dest] = swarm.physics.friction[src];
    swarm.physics.force[dest] = swarm.physics.force[src];
    swarm.physics.maxSpeed[dest] = swarm.physics.maxSpeed[src];
    swarm.physics.x[dest] = swarm.physics.x[src];
    swarm.physics.y[dest] = swarm.physics.y[src];
    swarm.physics.vx[dest] = swarm.physics.vx[src];
    swarm.physics.vy[dest] = swarm.physics.vy[src];
    swarm.physics.speed[dest] = swarm.physics.speed[src];
    swarm.physics.facingAngle[dest] = swarm.physics.facingAngle[src];
    
    swarm.perceptions.sensingRange[dest] = swarm.perceptions.sensingRange[src];
    swarm.perceptions.viewRadius[dest] = swarm.perceptions.viewRadius[src];
    swarm.perceptions.fovAngle[dest] = swarm.perceptions.fovAngle[src];
    swarm.perceptions.grassViewRadius[dest] = swarm.perceptions.grassViewRadius[src];
    
    swarm.sensors.lockedEnemyIndex[dest] = swarm.sensors.lockedEnemyIndex[src];
    swarm.sensors.closestEnemyIndex[dest] = swarm.sensors.closestEnemyIndex[src];
    swarm.sensors.closestEnemyX[dest] = swarm.sensors.closestEnemyX[src];
    swarm.sensors.closestEnemyY[dest] = swarm.sensors.closestEnemyY[src];
    swarm.sensors.closestEnemyDist[dest] = swarm.sensors.closestEnemyDist[src];
    swarm.sensors.enemyClosingSpeed[dest] = swarm.sensors.enemyClosingSpeed[src];
    swarm.sensors.enemyTangentialSpeed[dest] = swarm.sensors.enemyTangentialSpeed[src];
    swarm.sensors.foodSenseX[dest] = swarm.sensors.foodSenseX[src];
    swarm.sensors.foodSenseY[dest] = swarm.sensors.foodSenseY[src];
    swarm.sensors.foodDistance[dest] = swarm.sensors.foodDistance[src];
    swarm.sensors.foodClosingVelocity[dest] = swarm.sensors.foodClosingVelocity[src];
    swarm.sensors.foodTangentialVelocity[dest] = swarm.sensors.foodTangentialVelocity[src];
    swarm.sensors.energyReserve[dest] = swarm.sensors.energyReserve[src];
    
    swarm.neuralOutputs.thrustIntent[dest] = swarm.neuralOutputs.thrustIntent[src];
    swarm.neuralOutputs.turnIntent[dest] = swarm.neuralOutputs.turnIntent[src];
    swarm.neuralOutputs.previousThrustIntent[dest] = swarm.neuralOutputs.previousThrustIntent[src];
    swarm.neuralOutputs.previousTurnIntent[dest] = swarm.neuralOutputs.previousTurnIntent[src];
    
    for (int j = 0; j < W01_SIZE; j++) swarm.brains.w01[dest * W01_SIZE + j] = swarm.brains.w01[src * W01_SIZE + j];
    for (int j = 0; j < W12_SIZE; j++) swarm.brains.w12[dest * W12_SIZE + j] = swarm.brains.w12[src * W12_SIZE + j];
    for (int j = 0; j < B0_SIZE; j++) swarm.brains.b0[dest * B0_SIZE + j] = swarm.brains.b0[src * B0_SIZE + j];
    for (int j = 0; j < B1_SIZE; j++) swarm.brains.b1[dest * B1_SIZE + j] = swarm.brains.b1[src * B1_SIZE + j];
}

// Finalize count and reset metrics for the next tick
__global__ void finalizeCompactionKernel(SwarmData swarm) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        int alive = *swarm.compaction.alive_count;

        *swarm.current_count = alive;
        *swarm.compaction.birth_limit = alive;
        *swarm.compaction.alive_count = 0;
        *swarm.compaction.hole_count = 0;
        *swarm.compaction.mover_count = 0;
    }
}

void LifeSystem::handleDeaths(SwarmData& swarm, GraveyardData& graveyard, int active_agents){
    if (active_agents == 0) return;
    int grid_size = (active_agents + BLOCK_SIZE - 1) / BLOCK_SIZE;

    evaluateAndCountAliveKernel<<<grid_size, BLOCK_SIZE>>>(swarm, graveyard, active_agents);
    mapCompactionKernel<<<grid_size, BLOCK_SIZE>>>(swarm, active_agents);
    
    // worst-case grid_size so we don't have to sync the CPU to read hole_count
    moveCompactionKernel<<<grid_size, BLOCK_SIZE>>>(swarm, active_agents);
    
    finalizeCompactionKernel<<<1, 1>>>(swarm);
}

__global__ void handleBirthsKernel(SwarmData swarm, float mutationRate, float mutationStrength){
    
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= *swarm.compaction.birth_limit || !swarm.agentIdentifications.isAlive[i]) return;

    if (swarm.energyMetrics.energy[i] > MAX_ENERGY && swarm.energyMetrics.reproductionCooldown[i] <= 0.001f){
        if (*swarm.current_count >= swarm.max_capacity) return;

        int child_idx = atomicAdd(swarm.current_count, 1);
        // Prevent buffer overflow if the swarm maxes out 
        if (child_idx >= swarm.max_capacity) {
            atomicSub(swarm.current_count, 1); 
            return; 
        }

        float energyCost = BASE_REPRODUCTION_COST;
        swarm.energyMetrics.energy[i] -= energyCost * (1.0f + REPRODUCTION_COST_SCALING * swarm.energyMetrics.childCount[i]);
        swarm.energyMetrics.reproductionCooldown[i] = REPRODUCTION_COOLDOWN;
        swarm.energyMetrics.childCount[i]++;

        // New agent identification
        uint64_t world_prefix = static_cast<uint64_t>(swarm.world_id) << 48;
        unsigned long long int newLocalID = atomicAdd(reinterpret_cast<unsigned long long int*>(swarm.agentIdentifications.localAgentIDCounter), 1ULL);

        swarm.agentIdentifications.ID[child_idx] = world_prefix | static_cast<uint64_t>(newLocalID);
        swarm.agentIdentifications.speciesID[child_idx] = swarm.agentIdentifications.speciesID[i];
        swarm.agentIdentifications.isAlive[child_idx] = true;

        curandState localState = swarm.rng.state[i];

        float babyX = swarm.physics.x[i] + (curand_uniform(&localState) * 2.0f - 1.0f);
        float babyY = swarm.physics.y[i] + (curand_uniform(&localState) * 2.0f - 1.0f);

        // Thoroidal wrapping
        babyX = fmodf(babyX, (float)NUM_CELLE_X);
        if (babyX < 0) babyX += NUM_CELLE_X;
        babyY = fmodf(babyY, (float)NUM_CELLE_Y);
        if (babyY < 0) babyY += NUM_CELLE_Y;

        swarm.physics.x[child_idx] = babyX;
        swarm.physics.y[child_idx] = babyY;
        swarm.physics.vx[child_idx] = 0.0f;
        swarm.physics.vy[child_idx] = 0.0f;
        swarm.physics.speed[child_idx] = 0.0f;
        swarm.physics.facingAngle[child_idx] = (curand_uniform(&localState) * 2.0f - 1.0f) * CUDART_PI_F;

        // Inherits parent's characteristics
        swarm.physics.friction[child_idx] = swarm.physics.friction[i];
        swarm.physics.force[child_idx] = swarm.physics.force[i];
        swarm.physics.maxSpeed[child_idx] = swarm.physics.maxSpeed[i];
        
        swarm.perceptions.sensingRange[child_idx] = swarm.perceptions.sensingRange[i];
        swarm.perceptions.viewRadius[child_idx] = swarm.perceptions.viewRadius[i];
        swarm.perceptions.fovAngle[child_idx] = swarm.perceptions.fovAngle[i];
        swarm.perceptions.grassViewRadius[child_idx] = swarm.perceptions.grassViewRadius[i];

        // Energy metrics
        swarm.energyMetrics.energy[child_idx] = energyCost; 
        swarm.energyMetrics.digestionTime[child_idx] = swarm.energyMetrics.digestionTime[i];
        swarm.energyMetrics.remainingDigestion[child_idx] = 0.0f;
        swarm.energyMetrics.childCount[child_idx] = 0;
        swarm.energyMetrics.reproductionCooldown[child_idx] = 0.0f;

        swarm.fitnessMetrics.timeLived[child_idx] = 0.0f;
        swarm.fitnessMetrics.energyGained[child_idx] = 0.0f;

        // Neural outputs
        swarm.neuralOutputs.previousThrustIntent[child_idx] = 0.0f;
        swarm.neuralOutputs.previousTurnIntent[child_idx] = 0.0f;
        swarm.neuralOutputs.thrustIntent[child_idx] = 0.0f;
        swarm.neuralOutputs.turnIntent[child_idx] = 0.0f;

        // Brain copy and mutation
        const int W01_SIZE = INPUT_LAYER_SIZE * HIDDEN_LAYER_SIZE;
        const int W12_SIZE = HIDDEN_LAYER_SIZE * OUTPUT_LAYER_SIZE;
        const int B0_SIZE  = HIDDEN_LAYER_SIZE;
        const int B1_SIZE  = OUTPUT_LAYER_SIZE;

        // Copies parent's brain
        for (int j = 0; j < W01_SIZE; j++) swarm.brains.w01[child_idx * W01_SIZE + j] = swarm.brains.w01[i * W01_SIZE + j];
        for (int j = 0; j < W12_SIZE; j++) swarm.brains.w12[child_idx * W12_SIZE + j] = swarm.brains.w12[i * W12_SIZE + j];
        for (int j = 0; j < B0_SIZE; j++) swarm.brains.b0[child_idx * B0_SIZE + j] = swarm.brains.b0[i * B0_SIZE + j];
        for (int j = 0; j < B1_SIZE; j++) swarm.brains.b1[child_idx * B1_SIZE + j] = swarm.brains.b1[i * B1_SIZE + j];

        // Mutates the brain
        BrainSystem::mutateVectorDevice(swarm.brains.w01, child_idx * W01_SIZE, W01_SIZE, mutationRate, mutationStrength, &localState);
        BrainSystem::mutateVectorDevice(swarm.brains.w12, child_idx * W12_SIZE, W12_SIZE, mutationRate, mutationStrength, &localState);
        BrainSystem::mutateVectorDevice(swarm.brains.b0,  child_idx * B0_SIZE,  B0_SIZE, mutationRate, mutationStrength, &localState);
        BrainSystem::mutateVectorDevice(swarm.brains.b1,  child_idx * B1_SIZE,  B1_SIZE, mutationRate, mutationStrength, &localState);

        swarm.rng.state[i] = localState;
    }
}

// Hanles the birth of new agents
void LifeSystem::handleBirths(SwarmData& swarm, float mutationRate, float mutationStrength, int active_agents){
    if (active_agents == 0) return;

    int grid_size = (active_agents + BLOCK_SIZE - 1) / BLOCK_SIZE;

    handleBirthsKernel<<<grid_size, BLOCK_SIZE>>>(swarm, mutationRate, mutationStrength);
}


float LifeSystem::getFitness(SwarmData& swarm, int index){
    float fitness;
    if (swarm.agentIdentifications.speciesID[index] == PREY_ID) {
        fitness = swarm.fitnessMetrics.timeLived[index] + (swarm.fitnessMetrics.energyGained[index] * PREY_ENERGY_FITNESS_MULTIPLIER);

        if (swarm.energyMetrics.energy[index] > 0 && !swarm.agentIdentifications.isAlive[index]) {
            fitness *= PREY_HUNTED_PENALTY;
        }
    } else {
        fitness = swarm.fitnessMetrics.energyGained[index];
    }
    return fitness;
}