#pragma once
#include <vector>

using namespace std;

struct GraveyardData {
    int max_capacity;
    int* current_count;

    int* speciesID;
    float* fitness;

    float* w01;
    float* w12;
    float* b0;
    float* b1;

    void allocate(int capacity){
        max_capacity = capacity; 

        CUDA_CHECK(cudaMallocManaged(&current_count, sizeof(int)));
        CUDA_CHECK(cudaMallocManaged(&speciesID, capacity * sizeof(int)));
        CUDA_CHECK(cudaMallocManaged(&fitness, capacity * sizeof(float)));
        CUDA_CHECK(cudaMallocManaged(&w01, capacity * W01_SIZE * sizeof(float)));
        CUDA_CHECK(cudaMallocManaged(&w12, capacity * W12_SIZE * sizeof(float)));
        CUDA_CHECK(cudaMallocManaged(&b0, capacity * B0_SIZE * sizeof(float)));
        CUDA_CHECK(cudaMallocManaged(&b1, capacity * B1_SIZE * sizeof(float)));

        *current_count = 0;    
        CUDA_CHECK(cudaMemset(speciesID, 0, capacity * sizeof(int)));
        CUDA_CHECK(cudaMemset(fitness, 0, capacity * sizeof(float)));
        CUDA_CHECK(cudaMemset(w01, 0, capacity * W01_SIZE * sizeof(float)));
        CUDA_CHECK(cudaMemset(w12, 0, capacity * W12_SIZE * sizeof(float)));
        CUDA_CHECK(cudaMemset(b0, 0, capacity * B0_SIZE * sizeof(float)));
        CUDA_CHECK(cudaMemset(b1, 0, capacity * B1_SIZE * sizeof(float)));
    }

    void free(){
        CUDA_CHECK(cudaFree(current_count));
        CUDA_CHECK(cudaFree(speciesID));
        CUDA_CHECK(cudaFree(fitness));
        CUDA_CHECK(cudaFree(w01));
        CUDA_CHECK(cudaFree(w12));
        CUDA_CHECK(cudaFree(b0));
        CUDA_CHECK(cudaFree(b1));
    }
    
    GraveyardData(int capacity){
        allocate(capacity);
    }

    void clear() {
        *current_count = 0;
    }
};