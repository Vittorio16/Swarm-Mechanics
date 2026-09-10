#include <cuda_runtime.h>
#include "functions/LatticeSystem.h"
#include "Core/Config.h"

__global__ void latticeBuildKernel(SpatialLatticeData lattice, SwarmData swarm){
    // Fills the lattice with the current information
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i >= *swarm.current_count) return;

    int bx = (int)(swarm.physics.x[i] / LATTICE_CELL_WIDTH);
    int by = (int)(swarm.physics.y[i] / LATTICE_CELL_HEIGHT);
    bx = (bx % lattice.num_cells_x + lattice.num_cells_x) % lattice.num_cells_x;
    by = (by % lattice.num_cells_y + lattice.num_cells_y) % lattice.num_cells_y;

    int cell_index = by * lattice.num_cells_x + bx;
    
    // Adds the agent to the lattice if cell not full - needs to be atomic (kind of works like TSL, but adding 1 instead of locking)
    int current_count = atomicAdd(&lattice.cell_counts[cell_index], 1);
    if (current_count < MAX_AGENTS_PER_CELL) {
        int slot_index = (cell_index * MAX_AGENTS_PER_CELL) + current_count;
        lattice.cell_agent_indices[slot_index] = i;
    } else {
        // TODO: THROW AN ERROR
    }
}

void LatticeSystem::build(SpatialLatticeData& lattice, SwarmData& swarm){
    // Empties previous lattice
    CUDA_CHECK(cudaMemset(lattice.cell_counts, 0, lattice.total_cells * sizeof(int)));

    if (*swarm.current_count == 0) return;

    int grid_size = (*swarm.current_count + BLOCK_SIZE - 1) / BLOCK_SIZE;

    latticeBuildKernel<<<grid_size, BLOCK_SIZE>>>(lattice, swarm);
}