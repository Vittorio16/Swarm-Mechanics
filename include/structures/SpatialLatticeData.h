#pragma once
#include <cuda_runtime.h>
#include <thrust/device_ptr.h>
#include <thrust/fill.h>
#include "Core/Config.h"

using namespace std;

struct SpatialLatticeData{
    int num_cells_x;
    int num_cells_y;
    int total_cells;

    // Number of agents per cell
    int* cell_counts;
    // Array containing the indexes of the agents in the swarm
    int* cell_agent_indices;

    void allocate(int cx, int cy){
        num_cells_x = cx;
        num_cells_y = cy;
        total_cells = cx * cy;
        int total_slots = total_cells * MAX_AGENTS_PER_CELL;

        CUDA_CHECK(cudaMallocManaged(&cell_counts, total_cells * sizeof(int)));
        CUDA_CHECK(cudaMallocManaged(&cell_agent_indices, total_slots * sizeof(int)));
    
        CUDA_CHECK(cudaMemset(cell_counts, 0, total_cells * sizeof(int)));
        thrust::device_ptr<int> dev_ptr(cell_agent_indices);
        thrust::fill(dev_ptr, dev_ptr + total_slots, -1);
    }

    void free(){
        CUDA_CHECK(cudaFree(cell_counts));
        CUDA_CHECK(cudaFree(cell_agent_indices));
    }

    SpatialLatticeData(int cx, int cy){
        allocate(cx, cy);
    }
};