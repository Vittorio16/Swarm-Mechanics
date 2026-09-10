#include <algorithm>
#include "functions/FoodLatticeSystem.h"

namespace FoodLatticeSystem {
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_int_distribution<int> disX(0, NUM_CELLE_X - 1);
    thread_local std::uniform_int_distribution<int> disY(0, NUM_CELLE_Y - 1);
}

// Initializes the cuRAND states for the food lattice
__global__ void setupFoodCurandKernel(curandState* state, unsigned long seed, int max_capacity) {
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= max_capacity) return;
    curand_init(seed, i, 0, &state[i]);
}

void FoodLatticeSystem::initRNG(FoodLatticeData& foodLattice) {
    int grid_size = (MAX_SWARM_CAPACITY + BLOCK_SIZE - 1) / BLOCK_SIZE;
    setupFoodCurandKernel<<<grid_size, BLOCK_SIZE>>>(foodLattice.rng.state, std::random_device{}(), MAX_SWARM_CAPACITY);
    cudaDeviceSynchronize();
}

__global__ void growKernel(FoodLatticeData foodLattice, curandState* rngStates, float growthAmount){
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= *foodLattice.foodToSpawn) return; 

    curandState localState = rngStates[i];

    // Bounded Retry: if after MAX_SPAWN_ATTEMPTS we cannot respawn the food cell, we stop trying to
    for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; attempt++) {
        int cx = (int)(curand_uniform(&localState) * foodLattice.num_cells_x);
        int cy = (int)(curand_uniform(&localState) * foodLattice.num_cells_y);

        int grid_index = cy * foodLattice.num_cells_x + cx;

        // Food read in cell
        float expectedFloat = foodLattice.foodGrid[grid_index];

        // If it's empty, try to claim it
        if (expectedFloat <= 0.001f) {
            
            // Convert floats to ints for the atomicCAS
            int expectedBits = __float_as_int(expectedFloat);
            int newBits = __float_as_int(expectedFloat + growthAmount);
            
            // If the value in memory is still expectedBits, change it to newBits
            int oldBits = atomicCAS((int*)&foodLattice.foodGrid[grid_index], expectedBits, newBits);
            
            // If oldBits == expectedBits, successfully claimed it
            if (oldBits == expectedBits) {
                int chunk_x = cx / FOOD_CELL_WIDTH;
                int chunk_y = cy / FOOD_CELL_HEIGHT;
                int chunk_index = chunk_y * foodLattice.num_chunks_x + chunk_x;

                // float space_left = MAX_FOOD - current_food;
                // float actual_growth = min(growthAmount, space_left);
                float actual_growth = growthAmount;

                atomicAdd(&foodLattice.totalFood[chunk_index], actual_growth);
                atomicAdd(&foodLattice.sumFoodX[chunk_index], cx * actual_growth);
                atomicAdd(&foodLattice.sumFoodY[chunk_index], cy * actual_growth);
                
                break; 
            }
        }
    }
    rngStates[i] = localState;
}

// Tiny kernel to avoid having to sync to CPU
__global__ void resetFoodCounterKernel(int* foodToSpawn) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        *foodToSpawn = 0;
    }
}

void FoodLatticeSystem::grow(FoodLatticeData& foodLattice, float growthAmount){
    // We don't use foodToGrow, so that the CPU doesn't have to read it and we don't have to sync
    int grid_size = (MAX_SWARM_CAPACITY + BLOCK_SIZE - 1) / BLOCK_SIZE;

    growKernel<<<grid_size, BLOCK_SIZE>>>(foodLattice, foodLattice.rng.state, growthAmount);

    resetFoodCounterKernel<<<1, 1>>>(foodLattice.foodToSpawn);
}