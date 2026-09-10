#pragma once
#include "Core/Config.h"

using namespace std;

struct FoodLatticeData{
    struct RandData {
        curandState* state;
        void allocate(int capacity) {
            CUDA_CHECK(cudaMallocManaged(&state, capacity * sizeof(curandState)));
        }
        void free() {
            CUDA_CHECK(cudaFree(state));
        }
    } rng;

    // Global data
    int num_chunks_x;
    int num_chunks_y;
    int total_chunks;
    int* foodToSpawn;

    float* totalFood;
    float* sumFoodX;
    float*  sumFoodY;

    // Chunk data
    int num_cells_x;
    int num_cells_y;
    int total_cells;
    float* foodGrid;

    void allocate(int chunks_x, int chunks_y, int cells_x, int cells_y){
        num_chunks_x = chunks_x;
        num_chunks_y = chunks_y;
        total_chunks = chunks_x * chunks_y;

        rng.allocate(MAX_SWARM_CAPACITY);
        CUDA_CHECK(cudaMallocManaged(&foodToSpawn, sizeof(int)));
        CUDA_CHECK(cudaMallocManaged(&sumFoodX, chunks_x * chunks_y * sizeof(float)));
        CUDA_CHECK(cudaMallocManaged(&sumFoodY, chunks_x * chunks_y * sizeof(float)));
        CUDA_CHECK(cudaMallocManaged(&totalFood, chunks_x * chunks_y * sizeof(float)));

        *foodToSpawn = CONSTANT_FOOD_AMOUNT;
        CUDA_CHECK(cudaMemset(sumFoodX, 0, chunks_x * chunks_y * sizeof(float)));
        CUDA_CHECK(cudaMemset(sumFoodY, 0, chunks_x * chunks_y * sizeof(float)));
        CUDA_CHECK(cudaMemset(totalFood, 0, chunks_x * chunks_y * sizeof(float)));
        
        num_cells_x = cells_x;
        num_cells_y = cells_y;
        total_cells = cells_x * cells_y;

        CUDA_CHECK(cudaMallocManaged(&foodGrid, cells_x * cells_y * sizeof(float)));

        CUDA_CHECK(cudaMemset(foodGrid, 0, num_cells_x * num_cells_y * sizeof(float)));
    } 
    
    void free(){
        rng.free();
        CUDA_CHECK(cudaFree(foodToSpawn));
        CUDA_CHECK(cudaFree(sumFoodX));
        CUDA_CHECK(cudaFree(sumFoodY));
        CUDA_CHECK(cudaFree(totalFood));
        CUDA_CHECK(cudaFree(foodGrid));
    }

    FoodLatticeData(int chunks_x, int chunks_y, int cells_x, int cells_y){
        allocate(chunks_x, chunks_y, cells_x, cells_y);
    }
};