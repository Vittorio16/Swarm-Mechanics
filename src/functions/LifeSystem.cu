#include "Core/GlobalHelpers.h"
#include "Core/Physics.h"
#include "functions/LifeSystem.h"
#include "functions/BrainSystem.h"
#include <cuda_runtime.h>
#include "Core/GpuConfig.h"

__global__ void advanceTickKernel(uint64_t* tick){
    if (threadIdx.x == 0 && blockIdx.x == 0) (*tick)++;
}

void LifeSystem::advanceTick(SwarmData& swarm){
    advanceTickKernel<<<1, 1, 0, cudaStreamPerThread>>>(swarm.tick);
}

__global__ void createAgentsKernel(SwarmData swarm, int num_prey, int num_predators, uint64_t idBase){
    int count = *swarm.current_count;
    int stride = gridDim.x * blockDim.x;
    uint64_t tick = *swarm.tick;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
        int species = (i < num_prey) ? PREY_ID : PREDATOR_ID;

        RngStream rng(swarm.rng_seed, (uint64_t)i, tick, RngPurpose::AGENT_SPAWN);

        // Shift the world_id 48 bits to the left, leaving 48 bits for the local counter
        uint64_t world_prefix = static_cast<uint64_t>(swarm.world_id) << 48;
        swarm.agentIdentifications.ID[i] = world_prefix | (idBase + static_cast<uint64_t>(i));

        swarm.agentIdentifications.speciesID[i] = species;
        swarm.agentIdentifications.isAlive[i] = 1;

        swarm.physics.x[i] = rng.nextFloat() * (float)NUM_CELLE_X;
        swarm.physics.y[i] = rng.nextFloat() * (float)NUM_CELLE_Y;
        swarm.physics.facingAngle[i] = rng.nextFloat(-CUDART_PI_F, CUDART_PI_F);
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

        // On reset, the constructor doesn't initialize this fields since it's not called
        swarm.sensors.lockedEnemyIndex[i]       = NO_LOCKED_TARGET;
        swarm.sensors.closestEnemyIndex[i]      = -1;
        swarm.sensors.closestEnemyDist[i]       = 1.0f;
        swarm.sensors.closestEnemyX[i]          = 0.0f;
        swarm.sensors.closestEnemyY[i]          = 0.0f;
        swarm.sensors.enemyClosingSpeed[i]      = 0.0f;
        swarm.sensors.enemyTangentialSpeed[i]   = 0.0f;
        swarm.sensors.foodSenseX[i]             = 0.0f;
        swarm.sensors.foodSenseY[i]             = 0.0f;
        swarm.sensors.foodDistance[i]           = 1.0f;
        swarm.sensors.foodClosingVelocity[i]    = 0.0f;
        swarm.sensors.foodTangentialVelocity[i] = 0.0f;
        swarm.sensors.energyReserve[i]          = 0.0f;
        swarm.sensors.cachedFoodChunk[i]        = -1;
        swarm.neuralOutputs.thrustIntent[i]     = 0.0f;
        swarm.neuralOutputs.turnIntent[i]       = 0.0f;
    }
}

void LifeSystem::initSwarm(SwarmData& swarm, int num_prey, int num_predators){
    int total = num_prey + num_predators;
    if (total > swarm.max_capacity) {
        std::cerr << "initSwarm: requested " << total << " agents, clamping to "
                  << swarm.max_capacity << std::endl;
        float scale   = (float)swarm.max_capacity / (float)total;
        num_prey      = (int)(num_prey * scale);
        num_predators = swarm.max_capacity - num_prey;
        total         = swarm.max_capacity;
    }

    
    // Uses base to keep agentIds unique across generations
    uint64_t idBase = *swarm.agentIdentifications.localAgentIDCounter;

    *swarm.current_count = total;
    *swarm.agentIdentifications.localAgentIDCounter = idBase + (uint64_t)total;

    createAgentsKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm, num_prey, num_predators, idBase);
    
    CUDA_CHECK(cudaDeviceSynchronize());
    
    // Initializes random brains for the whole swarm
    BrainSystem::initRandom(swarm);
}

// Count survivors and write dead agents to the graveyard
__global__ void evaluateAndCountAliveKernel(SwarmData swarm, GraveyardData graveyard) {
    int count = *swarm.current_count;
    int stride = gridDim.x * blockDim.x;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
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
                    const int gcap = graveyard.max_capacity;
                    const int scap = swarm.max_capacity;

                    graveyard.speciesID[slot] = swarm.agentIdentifications.speciesID[i];
                    graveyard.fitness[slot] = fitness;

                    #pragma unroll
                    for (int j = 0; j < W01_SIZE; j++) graveyard.w01[j * gcap + slot] = swarm.brains.w01[j * scap + i];
                    #pragma unroll
                    for (int j = 0; j < W12_SIZE; j++) graveyard.w12[j * gcap + slot] = swarm.brains.w12[j * scap + i];
                    #pragma unroll
                    for (int j = 0; j < B0_SIZE; j++) graveyard.b0[j * gcap + slot] = swarm.brains.b0[j * scap + i];
                    #pragma unroll
                    for (int j = 0; j < B1_SIZE; j++) graveyard.b1[j * gcap + slot] = swarm.brains.b1[j * scap + i];
                } else {
                    atomicSub(graveyard.current_count, 1); // Graveyard full
                }
            }
        }
    }
}

// Map the Holes and Movers based on the boundary 
__global__ void mapCompactionKernel(SwarmData swarm) {
    int count = *swarm.current_count;
    int stride = gridDim.x * blockDim.x;
        
    int boundary = *swarm.compaction.alive_count;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
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
}

// Pair holes and movers up and relocate the data
__global__ void moveCompactionKernel(SwarmData swarm) {    
    // Only launch as many threads as there are holes
    int count = *swarm.compaction.hole_count;
    int stride = gridDim.x * blockDim.x;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
        int dest = swarm.compaction.holes_array[i];
        int src = swarm.compaction.movers_array[i];

        // Transfer all SoA data from src to dest
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
        swarm.sensors.cachedFoodChunk[dest] = swarm.sensors.cachedFoodChunk[src];
        swarm.sensors.foodSenseX[dest] = swarm.sensors.foodSenseX[src];
        swarm.sensors.foodSenseY[dest] = swarm.sensors.foodSenseY[src];
        swarm.sensors.foodDistance[dest] = swarm.sensors.foodDistance[src];
        swarm.sensors.foodClosingVelocity[dest] = swarm.sensors.foodClosingVelocity[src];
        swarm.sensors.foodTangentialVelocity[dest] = swarm.sensors.foodTangentialVelocity[src];
        swarm.sensors.energyReserve[dest] = swarm.sensors.energyReserve[src];
        
        swarm.neuralOutputs.thrustIntent[dest] = swarm.neuralOutputs.thrustIntent[src];
        swarm.neuralOutputs.turnIntent[dest] = swarm.neuralOutputs.turnIntent[src];
        
        const int scap = swarm.max_capacity;

        #pragma unroll
        for (int j = 0; j < W01_SIZE; j++) swarm.brains.w01[j * scap + dest] = swarm.brains.w01[j * scap + src];
        #pragma unroll
        for (int j = 0; j < W12_SIZE; j++) swarm.brains.w12[j * scap + dest] = swarm.brains.w12[j * scap + src];
        #pragma unroll
        for (int j = 0; j < B0_SIZE; j++) swarm.brains.b0[j * scap + dest] = swarm.brains.b0[j * scap + src];
        #pragma unroll
        for (int j = 0; j < B1_SIZE; j++) swarm.brains.b1[j * scap + dest] = swarm.brains.b1[j * scap + src];
    }
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

void LifeSystem::handleDeaths(SwarmData& swarm, GraveyardData& graveyard){
    evaluateAndCountAliveKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm, graveyard);
    mapCompactionKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm);
    
    // worst-case grid_size so we don't have to sync the CPU to read hole_count
    moveCompactionKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm);
    finalizeCompactionKernel<<<1, 1>>>(swarm);
}

__global__ void handleBirthsKernel(SwarmData swarm, float mutationRate, float mutationStrength){
    int count = *swarm.compaction.birth_limit;
    int stride = gridDim.x * blockDim.x;
    const int cap = swarm.max_capacity;
    const uint64_t tick = *swarm.tick;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
        if (!swarm.agentIdentifications.isAlive[i]) continue;

        if (swarm.energyMetrics.energy[i] > MAX_ENERGY && swarm.energyMetrics.reproductionCooldown[i] <= 0.001f){
            if (*swarm.current_count >= cap) continue;

            int child_idx = atomicAdd(swarm.current_count, 1);
            // Prevent buffer overflow if the swarm maxes out 
            if (child_idx >= cap) {
                atomicSub(swarm.current_count, 1); 
                continue; 
            }
            uint64_t parentID = swarm.agentIdentifications.ID[i];
            RngStream rng(swarm.rng_seed, parentID, tick, RngPurpose::REPRODUCTION);

            float energyCost = BASE_REPRODUCTION_COST;
            swarm.energyMetrics.energy[i] -= energyCost * (1.0f + REPRODUCTION_COST_SCALING * swarm.energyMetrics.childCount[i]);
            swarm.energyMetrics.reproductionCooldown[i] = REPRODUCTION_COOLDOWN;
            swarm.energyMetrics.childCount[i]++;

            // New agent identification
            uint64_t world_prefix = static_cast<uint64_t>(swarm.world_id) << 48;
            unsigned long long int newLocalID = atomicAdd(reinterpret_cast<unsigned long long int*>(swarm.agentIdentifications.localAgentIDCounter), 1ULL);
            uint64_t childID = world_prefix | static_cast<uint64_t>(newLocalID);

            swarm.agentIdentifications.ID[child_idx] = childID;
            swarm.agentIdentifications.speciesID[child_idx] = swarm.agentIdentifications.speciesID[i];
            swarm.agentIdentifications.isAlive[child_idx] = 1;

            float babyX = swarm.physics.x[i] + rng.nextFloat(-1.0f, 1.0f);
            float babyY = swarm.physics.y[i] + rng.nextFloat(-1.0f, 1.0f);

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
            swarm.physics.facingAngle[child_idx] = rng.nextFloat(-CUDART_PI_F, CUDART_PI_F);

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
            swarm.neuralOutputs.thrustIntent[child_idx] = 0.0f;
            swarm.neuralOutputs.turnIntent[child_idx] = 0.0f;

            // Copies parent's brain
            const int cap = swarm.max_capacity;
            #pragma unroll
            for (int j = 0; j < W01_SIZE; j++) swarm.brains.w01[j * cap + child_idx] = swarm.brains.w01[j * cap + i];
            #pragma unroll
            for (int j = 0; j < W12_SIZE; j++) swarm.brains.w12[j * cap + child_idx] = swarm.brains.w12[j * cap + i];
            #pragma unroll
            for (int j = 0; j < B0_SIZE; j++) swarm.brains.b0[j * cap + child_idx] = swarm.brains.b0[j * cap + i];
            #pragma unroll
            for (int j = 0; j < B1_SIZE; j++) swarm.brains.b1[j * cap + child_idx] = swarm.brains.b1[j * cap + i];

            // Separate stream keyed on the childs's brand-new ID. If this were keyed
            // on the parent, two children born to the same parent on the same tick
            // would receive identical mutations.
            RngStream mrng(swarm.rng_seed, childID, tick, RngPurpose::MUTATION);

            // Mutates the brain
            BrainSystem::mutateBrainDevice(swarm.brains.w01, W01_SIZE, child_idx, cap, mutationRate, mutationStrength, mrng);
            BrainSystem::mutateBrainDevice(swarm.brains.w12, W12_SIZE, child_idx, cap, mutationRate, mutationStrength, mrng);
            BrainSystem::mutateBrainDevice(swarm.brains.b0,  B0_SIZE,  child_idx, cap, mutationRate, mutationStrength, mrng);
            BrainSystem::mutateBrainDevice(swarm.brains.b1,  B1_SIZE,  child_idx, cap, mutationRate, mutationStrength, mrng);
        }
    }
}

// Hanles the birth of new agents
void LifeSystem::handleBirths(SwarmData& swarm, float mutationRate, float mutationStrength){
    handleBirthsKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm, mutationRate, mutationStrength);
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