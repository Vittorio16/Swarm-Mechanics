#include <cmath>
#include "functions/SensorySystem.h"
#include "Core/Physics.h"

__global__ void enemySenseKernel(SwarmData swarm, const SpatialLatticeData lattice, int active_agents){
    // Gets an observation of the closest enemy for each agent in the swarm
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= active_agents) return;

    float observerHeading = swarm.physics.facingAngle[i];
    int mySpecies = swarm.agentIdentifications.speciesID[i];

    uint64_t lockedIndex = swarm.sensors.lockedEnemyIndex[i];
    bool foundLockedTarget = false;

    // Used to keep track of "best" observation
    float minDistSq = INFINITY;
    int bestEnemyIndex = -1;
    float bestDist = INFINITY;
    // float bestAngleToTarget = 0.0f;
    float bestDx = 0.0f;
    float bestDy = 0.0f;

    // Checking the adjacent lattice cells
    int bx = (int)(swarm.physics.x[i] / LATTICE_CELL_WIDTH);
    int by = (int)(swarm.physics.y[i] / LATTICE_CELL_HEIGHT);
    
    bx = (bx % lattice.num_cells_x + lattice.num_cells_x) % lattice.num_cells_x;
    by = (by % lattice.num_cells_y + lattice.num_cells_y) % lattice.num_cells_y;
    
    int max_x_distance = (int)ceilf(swarm.perceptions.viewRadius[i] / LATTICE_CELL_WIDTH);
    int max_y_distance = (int)ceilf(swarm.perceptions.viewRadius[i] / LATTICE_CELL_HEIGHT);

    for (int h = - max_x_distance; h <= max_x_distance; h++){
        for (int j = -max_y_distance; j <= max_y_distance; j++){
            int temp_bx = ((bx + h) % lattice.num_cells_x + lattice.num_cells_x) % lattice.num_cells_x;
            int temp_by = ((by + j) % lattice.num_cells_y + lattice.num_cells_y) % lattice.num_cells_y;

            int cell_index = temp_by * lattice.num_cells_x + temp_bx;
            int agents_to_check = min(lattice.cell_counts[cell_index], MAX_AGENTS_PER_CELL);

            for (int cur_idx = 0; cur_idx < agents_to_check; cur_idx++){
                int idx_in_swarm = lattice.cell_agent_indices[cell_index * MAX_AGENTS_PER_CELL + cur_idx];

                if (idx_in_swarm == i || mySpecies == swarm.agentIdentifications.speciesID[idx_in_swarm]) continue;
    
                ThoroidalData coords = getThoroidalCoordinates(
                    swarm.physics.x[i], swarm.physics.y[i], swarm.physics.x[idx_in_swarm], swarm.physics.y[idx_in_swarm], NUM_CELLE_X, NUM_CELLE_Y
                );
        
                // If enemy isn't visible, continue
                if (coords.distSq > (swarm.perceptions.viewRadius[i] * swarm.perceptions.viewRadius[i]) && coords.distSq >= swarm.perceptions.sensingRange[i])
                    continue;
                
                if (coords.distSq > swarm.perceptions.sensingRange[i]){
                    float angleDiff = coords.angleToTarget - observerHeading;
                    if (isnan(angleDiff) || isinf(angleDiff)) angleDiff = 0.0f;
            
                    angleDiff = fmodf(angleDiff, 2*CUDART_PI_F);
                    if (angleDiff <= -CUDART_PI_F) angleDiff += 2 * CUDART_PI_F;
                    if (angleDiff > CUDART_PI_F) angleDiff -= 2 * CUDART_PI_F;
            
                    if (fabsf(angleDiff) > swarm.perceptions.fovAngle[i] * CUDART_PI_F / 360.0f){
                        continue; // Troppo di lato, non lo vede
                    }
                }

                // If prey is close enough to be eaten, it ignores tunnel vision
                if (mySpecies == PREDATOR_ID && coords.distSq <= KILL_RANGE_SQ) {
                    bestEnemyIndex = idx_in_swarm;
                    bestDx = coords.dx; bestDy = coords.dy; bestDist = coords.dist;
                    goto TARGET_FOUND; 
                }

                // Handles tunnel vision for predators
                if (lockedIndex != 0 && swarm.agentIdentifications.ID[idx_in_swarm] == lockedIndex && mySpecies != PREY_ID) {
                    bestEnemyIndex = idx_in_swarm;
                    bestDx = coords.dx; bestDy = coords.dy; bestDist = coords.dist;
                    foundLockedTarget = true; 
                    continue;
                }

                // Default
                if (!foundLockedTarget && coords.distSq < minDistSq) {
                    minDistSq = coords.distSq;
                    bestEnemyIndex = idx_in_swarm;
                    bestDx = coords.dx;
                    bestDy = coords.dy;
                    bestDist = coords.dist;
                }
            }
        }
    }
    TARGET_FOUND:
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
        swarm.sensors.lockedEnemyIndex[i] = 0;
        
        swarm.sensors.closestEnemyX[i] = 0.0f;
        swarm.sensors.closestEnemyY[i] = 0.0f;
        swarm.sensors.closestEnemyDist[i] = 1.0f;
        swarm.sensors.enemyClosingSpeed[i] = 0.0f;
        swarm.sensors.enemyTangentialSpeed[i] = 0.0f;
    }
}


__global__ void foodSenseKernel(SwarmData swarm, const FoodLatticeData foodLattice, int active_agents){
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= active_agents) return;

    // Gets the food scent for each agent
    // First finds the best food chunk
    float observerHeading = swarm.physics.facingAngle[i];

    int cx = (int)(swarm.physics.x[i] / FOOD_CELL_WIDTH);
    int cy = (int)(swarm.physics.y[i] / FOOD_CELL_HEIGHT);
    
    int max_distance = ceilf(swarm.perceptions.grassViewRadius[i] / FOOD_CELL_WIDTH);
    float bestFoodScore = -1.0f;
    int bestChunkIndex = -1;

    // Loop through nearby chunks in the food lattice
    for (int h = -max_distance; h <= max_distance; h++) {
        for (int j = -max_distance; j <= max_distance; j++) {
            int tx = ((cx + h) % foodLattice.num_chunks_x + foodLattice.num_chunks_x) % foodLattice.num_chunks_x;
            int ty = ((cy + j) % foodLattice.num_chunks_y + foodLattice.num_chunks_y) % foodLattice.num_chunks_y;

            int chunk_index = ty * foodLattice.num_chunks_x + tx;   

            if (foodLattice.totalFood[chunk_index] > 0.001f){
                float centerX = foodLattice.sumFoodX[chunk_index] / foodLattice.totalFood[chunk_index];
                float centerY = foodLattice.sumFoodY[chunk_index] / foodLattice.totalFood[chunk_index];

                // Calculate distance from agent to this chunk's center of mass
                float dx = centerX - swarm.physics.x[i];
                float dy = centerY - swarm.physics.y[i];

                // Handle wrapping for distance calculation
                if (dx > NUM_CELLE_X * 0.5f) dx -= NUM_CELLE_X;
                if (dx < -NUM_CELLE_X * 0.5f) dx += NUM_CELLE_X;

                if (dy > NUM_CELLE_Y * 0.5f) dy -= NUM_CELLE_Y;
                if (dy < -NUM_CELLE_Y * 0.5f) dy += NUM_CELLE_Y;

                float distSq = dx*dx + dy*dy;
                if (distSq < 0.1f) distSq = 0.1f;

                if (distSq < swarm.perceptions.grassViewRadius[i] * swarm.perceptions.grassViewRadius[i]){
                    float foodScore = foodLattice.totalFood[chunk_index]*foodLattice.totalFood[chunk_index] / distSq;

                    if (foodScore > bestFoodScore) {
                        bestFoodScore = foodScore;
                        bestChunkIndex = chunk_index;
                    }
                }
            }
        }
    }

    // Now loop inside the best chunk to find the best cell, to improve precision when close to food
    float bestCellFoodScore = -1.0f;
    float bestCellDx = 0.0f;
    float bestCellDy = 0.0f;

    // Safe Control Flow: Only refine if a chunk was actually found
    if (bestChunkIndex != -1) {
        int chunk_x = bestChunkIndex % foodLattice.num_chunks_x;
        int chunk_y = bestChunkIndex / foodLattice.num_chunks_x;
        int start_cell_x = chunk_x * FOOD_CELL_WIDTH;
        int start_cell_y = chunk_y * FOOD_CELL_HEIGHT;
        
        // Cache the squared radius to avoid recalculating it 100 times
        float viewRadiusSq = swarm.perceptions.grassViewRadius[i] * swarm.perceptions.grassViewRadius[i];

        for (int cell_dx = 0; cell_dx < FOOD_CELL_WIDTH; cell_dx++) {
            for (int cell_dy = 0; cell_dy < FOOD_CELL_HEIGHT; cell_dy++) {
                
                int cell_x = start_cell_x + cell_dx;
                int cell_y = start_cell_y + cell_dy;
                
                // Indeces for flat food grid
                int cell_index = cell_y * foodLattice.num_cells_x + cell_x;
                
                float foodAmount = foodLattice.foodGrid[cell_index];
                if (foodAmount <= 0.001f) continue;

                // Thoroidal distance calculation
                float dx = cell_x + 0.5f - swarm.physics.x[i];
                float dy = cell_y + 0.5f - swarm.physics.y[i];

                // Handle wrapping for distance calculation
                if (dx > NUM_CELLE_X * 0.5f) dx -= NUM_CELLE_X;
                if (dx < -NUM_CELLE_X * 0.5f) dx += NUM_CELLE_X;
                if (dy > NUM_CELLE_Y * 0.5f) dy -= NUM_CELLE_Y;
                if (dy < -NUM_CELLE_Y * 0.5f) dy += NUM_CELLE_Y;

                float distSq = dx*dx + dy*dy;
                if (distSq < 0.1f) distSq = 0.1f;

                // Ignore cells outside the view radius
                if (distSq <= viewRadiusSq) {
                    float foodScore = foodAmount * foodAmount / distSq;
                    
                    if (foodScore > bestCellFoodScore) {
                        bestCellFoodScore = foodScore;
                        bestCellDx = dx;
                        bestCellDy = dy;
                    }
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
        float normalization = swarm.perceptions.grassViewRadius[i];
        
        float distToFood = hypotf(bestFoodX, bestFoodY);
        if (distToFood < 0.001f) distToFood = 0.001f;

        // Normalize the food sense vector, dividing the direction vector from the distance
        float normFoodX = bestFoodX / distToFood;
        float normFoodY = bestFoodY / distToFood;
        
        swarm.sensors.foodSenseX[i] = normFoodX;
        swarm.sensors.foodSenseY[i] = normFoodY;
    
        float foodDistance = distToFood / normalization;
        swarm.sensors.foodDistance[i] = foodDistance;
    
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

// Gets an observation of the closest enemy, of food and parses it into the sensors
void SensorySystem::update(SwarmData& swarm, const SpatialLatticeData& lattice, const FoodLatticeData& foodLattice, int active_agents){
    if (active_agents == 0) return;

    int grid_size = (active_agents + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    enemySenseKernel<<<grid_size, BLOCK_SIZE>>>(swarm, lattice, active_agents);
    foodSenseKernel<<<grid_size, BLOCK_SIZE>>>(swarm, foodLattice, active_agents);
}