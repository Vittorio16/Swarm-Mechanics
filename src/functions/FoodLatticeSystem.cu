#include <algorithm>
#include "functions/FoodLatticeSystem.h"
#include "Core/GpuConfig.h"
#include "Core/Rng.h"

__global__ void buildChunkSummaryKernel(FoodLatticeData foodLattice){
    int stride = gridDim.x * blockDim.x;

    for (int c = blockIdx.x * blockDim.x + threadIdx.x;
         c < foodLattice.total_chunks; c += stride) {

        float total = foodLattice.totalFood[c];

        ChunkSummary s;
        s.totalFood = total;
        if (total > 0.001f) {
            s.comX = foodLattice.sumFoodX[c] / total;
            s.comY = foodLattice.sumFoodY[c] / total;
        } else {
            s.comX = 0.0f;
            s.comY = 0.0f;
        }
        s._pad = 0.0f;

        foodLattice.chunkSummary[c] = s;
    }
}

// Creates chunk memory
void FoodLatticeSystem::buildChunkSummary(FoodLatticeData& foodLattice){
    buildChunkSummaryKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(foodLattice);
}

__global__ void growKernel(FoodLatticeData foodLattice, float growthAmount, uint64_t seed, const uint64_t* tickPtr){
    int count = *foodLattice.foodToSpawn;
    int stride = gridDim.x * blockDim.x;
    uint64_t tick = *tickPtr;

    for (int i = blockIdx.x * blockDim.x + threadIdx.x; i < count; i += stride){
        RngStream rng(seed, (uint64_t)i, tick, RngPurpose::FOOD_SPAWN);

        // Bounded Retry: if after MAX_SPAWN_ATTEMPTS we cannot respawn the food cell, we stop trying to
        for (int attempt = 0; attempt < MAX_SPAWN_ATTEMPTS; attempt++) {
            int cx = rng.nextInt(foodLattice.num_cells_x);
            int cy = rng.nextInt(foodLattice.num_cells_y);

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
    }
}

// Tiny kernel to avoid having to sync to CPU
__global__ void resetFoodCounterKernel(int* foodToSpawn) {
    if (threadIdx.x == 0 && blockIdx.x == 0) {
        *foodToSpawn = 0;
    }
}

void FoodLatticeSystem::grow(FoodLatticeData& foodLattice, float growthAmount, uint64_t seed, const uint64_t* tick){
    // We don't use foodToGrow, so that the CPU doesn't have to read it and we don't have to sync
    growKernel<<<GpuConfig::persistentGrid, BLOCK_SIZE, 0, cudaStreamPerThread>>>(foodLattice, growthAmount, seed, tick);
    resetFoodCounterKernel<<<1, 1>>>(foodLattice.foodToSpawn);
}