#include <cmath>
#include "functions/SensorySystem.h"
#include "Core/Physics.h"
#include "Core/GpuConfig.h"

__global__ void enemySenseKernel(SwarmData swarm, const SpatialLatticeData lattice){
    // Gets an observation of the closest enemy for each agent in the swarm
    int count = *swarm.current_count;
    int stride = gridDim.x * blockDim.x;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
        float observerHeading = swarm.physics.facingAngle[i];
        int mySpecies = swarm.agentIdentifications.speciesID[i];

        // Useful constants
        const float viewRadiusSq = swarm.perceptions.viewRadius[i] * swarm.perceptions.viewRadius[i];
        const float sensingSq = swarm.perceptions.sensingRange[i];
        const float myX = swarm.physics.x[i];
        const float myY = swarm.physics.y[i];
        const float halfFov = swarm.perceptions.fovAngle[i] * (CUDART_PI_F / 360.0f);

        uint64_t lockedIndex = swarm.sensors.lockedEnemyIndex[i];
        bool foundLockedTarget = false;

        // Used to keep track of "best" observation
        float minDistSq = INFINITY;
        int bestEnemyIndex = -1;
        float bestDistSq = INFINITY;
        float bestDist = INFINITY;
        // float bestAngleToTarget = 0.0f;
        float bestDx = 0.0f;
        float bestDy = 0.0f;

        // Checking the adjacent lattice cells
        int bx = (int)(swarm.physics.x[i] / LATTICE_CELL_WIDTH);
        int by = (int)(swarm.physics.y[i] / LATTICE_CELL_HEIGHT);
        
        bx = (bx % lattice.num_cells_x + lattice.num_cells_x) % lattice.num_cells_x;
        by = (by % lattice.num_cells_y + lattice.num_cells_y) % lattice.num_cells_y;
        
        float reach = fmaxf(swarm.perceptions.viewRadius[i], sqrtf(sensingSq));

        int max_x_distance = (int)ceilf(reach / LATTICE_CELL_WIDTH);
        int max_y_distance = (int)ceilf(reach / LATTICE_CELL_HEIGHT);

        for (int h = - max_x_distance; h <= max_x_distance; h++){
            for (int j = -max_y_distance; j <= max_y_distance; j++){
                int temp_bx = ((bx + h) % lattice.num_cells_x + lattice.num_cells_x) % lattice.num_cells_x;
                int temp_by = ((by + j) % lattice.num_cells_y + lattice.num_cells_y) % lattice.num_cells_y;

                int cell_index = temp_by * lattice.num_cells_x + temp_bx;
                int agents_to_check = min(lattice.cell_counts[cell_index], MAX_AGENTS_PER_CELL);

                for (int cur_idx = 0; cur_idx < agents_to_check; cur_idx++){
                    int idx_in_swarm = lattice.cell_agent_indices[cell_index * MAX_AGENTS_PER_CELL + cur_idx];

                    if (idx_in_swarm == i || mySpecies == swarm.agentIdentifications.speciesID[idx_in_swarm]) continue;
        
                    ThoroidalDelta d = thoroidalDelta(myX, myY, swarm.physics.x[idx_in_swarm], swarm.physics.y[idx_in_swarm], NUM_CELLE_X, NUM_CELLE_Y);
                    
                    // If enemy isn't visible, continue
                    bool inProximity = (d.distSq < sensingSq);
                    bool inViewCone = (d.distSq < viewRadiusSq);
                    if (!inProximity && !inViewCone) continue;

                    // Has 360 view in proximity
                    if (!inProximity) {
                        float angleDiff = atan2f(d.dy, d.dx) - observerHeading;
                        angleDiff = fmodf(angleDiff, 2.0f * CUDART_PI_F);
                        if (angleDiff <= -CUDART_PI_F) angleDiff += 2.0f * CUDART_PI_F;
                        if (angleDiff  >  CUDART_PI_F) angleDiff -= 2.0f * CUDART_PI_F;
                        if (fabsf(angleDiff) > halfFov) continue;
                    }

                    // If prey is close enough to be eaten, it ignores tunnel vision
                    if (mySpecies == PREDATOR_ID && d.distSq <= KILL_RANGE_SQ) {
                        bestEnemyIndex = idx_in_swarm;
                        bestDx = d.dx; bestDy = d.dy;
                        bestDistSq = d.distSq;
                        goto TARGET_FOUND; 
                    }

                    // Handles tunnel vision for predators
                    if (lockedIndex != NO_LOCKED_TARGET && swarm.agentIdentifications.ID[idx_in_swarm] == lockedIndex && mySpecies != PREY_ID) {
                        bestEnemyIndex = idx_in_swarm;
                        bestDx = d.dx; bestDy = d.dy;
                        bestDistSq = d.distSq;
                        foundLockedTarget = true; 
                        continue;
                    }

                    // Default
                    if (!foundLockedTarget && d.distSq < minDistSq) {
                        minDistSq = d.distSq;
                        bestEnemyIndex = idx_in_swarm;
                        bestDx = d.dx;
                        bestDy = d.dy;
                        bestDistSq = d.distSq;
                    }
                }
            }
        }
        TARGET_FOUND:
        if (bestEnemyIndex != -1) {
            bestDist = sqrtf(bestDistSq);
        }

        // Updates the agent's sensory data with the observation
        swarm.sensors.closestEnemyIndex[i] = bestEnemyIndex;

        swarm.sensors.energyReserve[i] = 1 - swarm.energyMetrics.energy[i] / MAX_ENERGY;

        // Terminate early if no enemies are visible
        if (bestEnemyIndex != -1){
            swarm.sensors.lockedEnemyIndex[i] = swarm.agentIdentifications.ID[bestEnemyIndex];

            float c = cosf(-observerHeading);
            float s = sinf(-observerHeading);
            
            float localX = bestDx * c - bestDy * s;
            float localY = bestDx * s + bestDy * c;

            if (bestDist <= 0.001f){
                swarm.sensors.closestEnemyX[i] = 0;
                swarm.sensors.closestEnemyY[i] = 0;
            } else {
                swarm.sensors.closestEnemyX[i] = localX / bestDist;
                swarm.sensors.closestEnemyY[i] = localY / bestDist;
            }

            swarm.sensors.closestEnemyDist[i] = bestDist / swarm.perceptions.viewRadius[i];

            float relvx = swarm.physics.vx[bestEnemyIndex] - swarm.physics.vx[i];
            float relvy = swarm.physics.vy[bestEnemyIndex] - swarm.physics.vy[i];

            float vLongitudinal = relvx * c - relvy * s;
            float vTangential = relvx * s + relvy * c;

            // Normalize 
            swarm.sensors.enemyClosingSpeed[i] = vLongitudinal / swarm.physics.maxSpeed[bestEnemyIndex];
            swarm.sensors.enemyTangentialSpeed[i] = vTangential / swarm.physics.maxSpeed[bestEnemyIndex];
        } else {
            swarm.sensors.lockedEnemyIndex[i] = NO_LOCKED_TARGET;
            
            swarm.sensors.closestEnemyX[i] = 0.0f;
            swarm.sensors.closestEnemyY[i] = 0.0f;
            swarm.sensors.closestEnemyDist[i] = 1.0f;
            swarm.sensors.enemyClosingSpeed[i] = 0.0f;
            swarm.sensors.enemyTangentialSpeed[i] = 0.0f;
        }
    }
}


__global__ void foodSenseKernel(SwarmData swarm, const FoodLatticeData foodLattice){
    int count = *swarm.current_count;
    int stride = gridDim.x * blockDim.x;
    const uint64_t tick = *swarm.tick;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
        // Gets the food scent for each agent
        // First finds the best food chunk
        const float myX = swarm.physics.x[i];
        const float myY = swarm.physics.y[i];
        float observerHeading = swarm.physics.facingAngle[i];

        const float grassRadius   = swarm.perceptions.grassViewRadius[i];
        const float grassRadiusSq = grassRadius * grassRadius;

        int bestChunkIndex = swarm.sensors.cachedFoodChunk[i];

        // Only search for food evey FOOD_CHUNK_REFRESH_INTERVAL, or if the cached location was eaten
        bool needsRefresh = (bestChunkIndex < 0)
                         || (((tick + (uint64_t)i) % FOOD_CHUNK_REFRESH_INTERVAL) == 0)
                         || (foodLattice.chunkSummary[bestChunkIndex].totalFood <= 0.001f);

        if (needsRefresh){            
            int cx = (int)(swarm.physics.x[i] / FOOD_CELL_WIDTH);
            int cy = (int)(swarm.physics.y[i] / FOOD_CELL_HEIGHT);
            
            int maxChunkDist = (int)ceilf(grassRadius / FOOD_CELL_WIDTH);
            float bestFoodScore = -1.0f;
            int bestChunkIndex = -1;

            // Loop through nearby chunks in the food lattice
            for (int h = -maxChunkDist; h <= maxChunkDist; h++) {
                for (int j = -maxChunkDist; j <= maxChunkDist; j++) {
                    int tx = ((cx + h) % foodLattice.num_chunks_x + foodLattice.num_chunks_x) % foodLattice.num_chunks_x;
                    int ty = ((cy + j) % foodLattice.num_chunks_y + foodLattice.num_chunks_y) % foodLattice.num_chunks_y;

                    int c = ty * foodLattice.num_chunks_x + tx;   

                    ChunkSummary s = foodLattice.chunkSummary[c];

                    if (s.totalFood <= 0.001f) continue;
                
                    ThoroidalDelta d = thoroidalDelta(myX, myY, s.comX, s.comY, NUM_CELLE_X, NUM_CELLE_Y);
                    float distSq = fmaxf(d.distSq, 0.1f);

                    if (distSq >= grassRadiusSq) continue;

                    float foodScore = s.totalFood * s.totalFood / distSq;

                    if (foodScore > bestFoodScore) {
                        bestFoodScore = foodScore;
                        bestChunkIndex = c;
                    }
                }
            }
            swarm.sensors.cachedFoodChunk[i] = bestChunkIndex;
        }

        // Now loop inside the best chunk to find the best cell, to improve precision when close to food
        float bestCellFoodScore = -1.0f;
        float bestCellDx = 0.0f;
        float bestCellDy = 0.0f;

        // Safe Control Flow: Only refine if a chunk was actually found
        if (bestChunkIndex >= 0) {
            int chunk_x = bestChunkIndex % foodLattice.num_chunks_x;
            int chunk_y = bestChunkIndex / foodLattice.num_chunks_x;
            int start_cell_x = chunk_x * FOOD_CELL_WIDTH;
            int start_cell_y = chunk_y * FOOD_CELL_HEIGHT;
            
            for (int cell_dy = 0; cell_dy < FOOD_CELL_HEIGHT; cell_dy++) {
                int cell_y = start_cell_y + cell_dy;
                int row    = cell_y * foodLattice.num_cells_x;

                for (int cell_dx = 0; cell_dx < FOOD_CELL_WIDTH; cell_dx++) {
                    int cell_x = start_cell_x + cell_dx;

                    float foodAmount = foodLattice.foodGrid[row + cell_x];
                    if (foodAmount <= 0.001f) continue;

                    ThoroidalDelta d = thoroidalDelta(myX, myY,
                                                      cell_x + 0.5f, cell_y + 0.5f,
                                                      NUM_CELLE_X, NUM_CELLE_Y);

                    float distSq = fmaxf(d.distSq, 0.1f);
                    if (distSq > grassRadiusSq) continue;

                    float score = foodAmount * foodAmount / distSq;
                    if (score > bestCellFoodScore) {
                        bestCellFoodScore = score;
                        bestCellDx = d.dx;
                        bestCellDy = d.dy;
                    }
                }
            }
        }

        if (bestCellFoodScore >= 0.0f){
            // Rotate to agent's coordinates
            float c = cosf(-observerHeading);
            float s = sinf(-observerHeading);

            float bestFoodX = bestCellDx * c - bestCellDy * s;
            float bestFoodY = bestCellDx * s + bestCellDy * c;

            // Grass inputs  
            float distToFood = fmaxf(sqrtf(bestFoodX * bestFoodX + bestFoodY * bestFoodY), 0.001f);

            // Normalize the food sense vector, dividing the direction vector from the distance
            float normFoodX = bestFoodX / distToFood;
            float normFoodY = bestFoodY / distToFood;
            
            swarm.sensors.foodSenseX[i] = normFoodX;
            swarm.sensors.foodSenseY[i] = normFoodY;
            swarm.sensors.foodDistance[i] = fminf(1.0f, distToFood / grassRadius);
        
            // Calculate the agent's heading for closing and tangential velocity calculations
            // Maybe needless to use facingAngle
            // float c = cosf(-swarm.physics.facingAngle[i]);
            // float s = sinf(-swarm.physics.facingAngle[i]);
        
            float vLongitudinal = swarm.physics.vx[i] * c - swarm.physics.vy[i] * s;
            float vTangential = swarm.physics.vx[i] * s + swarm.physics.vy[i] * c;
        
            float closingVelocity = vLongitudinal * normFoodX + vTangential * normFoodY;
            float tangentialVelocity = vLongitudinal * normFoodY - vTangential * normFoodX;
        
            swarm.sensors.foodClosingVelocity[i] = closingVelocity / swarm.physics.maxSpeed[i];
            swarm.sensors.foodTangentialVelocity[i] = tangentialVelocity / swarm.physics.maxSpeed[i];
        } else {
            swarm.sensors.foodSenseX[i] = 0.0f;
            swarm.sensors.foodSenseY[i] = 0.0f;
            swarm.sensors.foodDistance[i] = 1.0f;
            swarm.sensors.foodClosingVelocity[i] = 0.0f;
            swarm.sensors.foodTangentialVelocity[i] = 0.0f;
        }
    }
}

// Gets an observation of the closest enemy, of food and parses it into the sensors
void SensorySystem::update(SwarmData& swarm, const SpatialLatticeData& lattice, const FoodLatticeData& foodLattice){
    enemySenseKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm, lattice);
    foodSenseKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(swarm, foodLattice);
}