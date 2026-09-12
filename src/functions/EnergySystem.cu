#include "functions/EnergySystem.h"
#include "functions/FoodLatticeSystem.h"
#include "Core/Physics.h"
#include "Core/GpuConfig.h"

__global__ void energySystemUpdateKernel(SwarmData swarm, FoodLatticeData foodLattice, float dt){
    int count = *swarm.current_count;
    int stride = gridDim.x * blockDim.x;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
        if (swarm.energyMetrics.remainingDigestion[i] > 0.0f){
            swarm.energyMetrics.remainingDigestion[i] -= dt;
            if (swarm.energyMetrics.remainingDigestion[i] <= 0.001f) swarm.energyMetrics.remainingDigestion[i] = 0; 
        }
        if (swarm.energyMetrics.reproductionCooldown[i] > 0){
            swarm.energyMetrics.reproductionCooldown[i] -= dt;
            if (swarm.energyMetrics.reproductionCooldown[i] < 0) swarm.energyMetrics.reproductionCooldown[i] = 0;
        }

        float actionEnergyCost = fabsf(swarm.neuralOutputs.thrustIntent[i]) + fabsf(swarm.neuralOutputs.turnIntent[i]) * TURNING_COST_PENALTY;

        float metabolismCost = METABOLISM_COST;
        metabolismCost *= swarm.agentIdentifications.speciesID[i] == PREY_ID ? PREY_METABOLISM_MULTIPLIER : PREDATOR_METABOLISM_MULTIPLIER;

        float effortCost = actionEnergyCost * MAX_EFFORT_COST;
        effortCost *= swarm.agentIdentifications.speciesID[i] == PREY_ID ? PREY_EFFORT_MULTIPLIER : PREDATOR_EFFORT_MULTIPLIER;
        
        float energyLoss = metabolismCost * dt + effortCost * dt;
        swarm.energyMetrics.energy[i] -= energyLoss;

        if (swarm.energyMetrics.remainingDigestion[i] > 0.001f) continue;

        // Feeding mechanics
        if (swarm.agentIdentifications.speciesID[i] == PREY_ID){
            // Prey gain energy by eating grass 
        
            float posX = swarm.physics.x[i];
            float posY = swarm.physics.y[i];
            if (isnan(posX) || isinf(posX)) posX = 0.0f;
            if (isnan(posY) || isinf(posY)) posY = 0.0f;
            
            int centerIndexX = (int)posX;
            int centerIndexY = (int)posY;
        
            // Check the 3x3 grid around the agent
            for (int dx = -1; dx <= 1; dx++) {
                for (int dy = -1; dy <= 1; dy++) {
                    
                    // Calculate Neighbor Coordinates (with wrapping)
                    int tx = centerIndexX + dx;
                    int ty = centerIndexY + dy;
                    
                    tx = ((tx % NUM_CELLE_X) + NUM_CELLE_X) % NUM_CELLE_X;
                    ty = ((ty % NUM_CELLE_Y) + NUM_CELLE_Y) % NUM_CELLE_Y;
                    int cell_index = ty * foodLattice.num_cells_x + tx;

                    // Skip empty cells
                    if (foodLattice.foodGrid[cell_index] <= 0.001f) continue;
        
                    // Calculate Distance to the centre of that target cell
                    float fracX = swarm.physics.x[i] - (int)swarm.physics.x[i]; 
                    float fracY = swarm.physics.y[i] - (int)swarm.physics.y[i];
                    
                    float vecX = dx - fracX + 0.5f;
                    float vecY = dy - fracY + 0.5f;
        
                    float distSq = vecX*vecX + vecY*vecY;
        
                    // Prey begins digesting and gains energy based on the food eaten
                    if (distSq < PREY_EAT_RADIUS_SQ) {
                        // Avoids race condition of multiple prey eating the same grass
                        float foodEaten = atomicExch(&foodLattice.foodGrid[cell_index], 0.0f);
                        if (foodEaten > 0.001f){
                            atomicAdd(foodLattice.foodToSpawn, 1);

                            swarm.energyMetrics.remainingDigestion[i] = swarm.energyMetrics.digestionTime[i];
                            swarm.fitnessMetrics.energyGained[i] += foodEaten;
                            swarm.energyMetrics.energy[i] += foodEaten; 
                        
                            // Update the food lattice
                            int cx = tx / FOOD_CELL_WIDTH;
                            int cy = ty / FOOD_CELL_HEIGHT;
                            int chunk_index = cy * foodLattice.num_chunks_x + cx;
            
                            atomicAdd(&foodLattice.totalFood[chunk_index],  -foodEaten); 
                            atomicAdd(&foodLattice.sumFoodX[chunk_index], -(tx * foodEaten));
                            atomicAdd(&foodLattice.sumFoodY[chunk_index], -(ty * foodEaten));

                            goto FINISH_EATING;
                        }
                    }
                }
            }
            FINISH_EATING: ;
        } else {
            // Predators gain energy by eating prey
            if (swarm.sensors.closestEnemyIndex[i] != -1 && swarm.agentIdentifications.isAlive[swarm.sensors.closestEnemyIndex[i]]){
                // Check if the closest enemy is within kill range
                int t = swarm.sensors.closestEnemyIndex[i];
                if (t != -1 && swarm.agentIdentifications.isAlive[t]) {
                    // Calculate wrapped distance 
                    float dx = swarm.physics.x[t] - swarm.physics.x[i];
                    float dy = swarm.physics.y[t] - swarm.physics.y[i];
                    
                    // Thoroidal Wrapping
                    if (dx > NUM_CELLE_X * 0.5f) dx -= NUM_CELLE_X;
                    else if (dx < -NUM_CELLE_X * 0.5f) dx += NUM_CELLE_X;
                    
                    if (dy > NUM_CELLE_Y * 0.5f) dy -= NUM_CELLE_Y;
                    else if (dy < -NUM_CELLE_Y * 0.5f) dy += NUM_CELLE_Y;
                    
                    float distSq = dx*dx + dy*dy;

                    if (distSq < KILL_RANGE_SQ) {
                        // Avoids race condition of multiple predators eating the same prey
                        int wasAlive = atomicExch((int*)&swarm.agentIdentifications.isAlive[t], 0);
                        if (wasAlive == 1) {
                            swarm.energyMetrics.energy[i] += PREDATOR_ENERGY_GAIN;
                            swarm.fitnessMetrics.energyGained[i] += PREDATOR_ENERGY_GAIN;
                            swarm.energyMetrics.remainingDigestion[i] = swarm.energyMetrics.digestionTime[i];
                        }
                    }
                }
            }
        }

        // Cap on energy
        if (swarm.energyMetrics.energy[i] > 3.0f * MAX_ENERGY / 2.0f) swarm.energyMetrics.energy[i] = 3.0f * MAX_ENERGY / 2.0f;
        if (swarm.energyMetrics.energy[i] <= 0.0f) swarm.agentIdentifications.isAlive[i] = false;
    }
}

// Handles energy consumption and feeding
void EnergySystem::update(SwarmData& swarm, FoodLatticeData& foodLattice, float dt){
    energySystemUpdateKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm, foodLattice, dt);
}