#include "GPU/functions/LatticeSystem.h"

void LatticeSystem::build(SpatialLatticeData& lattice, SwarmData& swarm){
    // Empties previous lattice
    fill(lattice.cell_counts.begin(), lattice.cell_counts.end(), 0);

    // Fills the lattice with the current information
    for (int i = 0; i < swarm.current_count; i++) {
        if (!swarm.agentIdentifications.isAlive[i]) continue;

        int bx = (int)(swarm.physics.x[i] / LATTICE_CELL_WIDTH);
        int by = (int)(swarm.physics.y[i] / LATTICE_CELL_HEIGHT);
        bx = (bx % lattice.num_cells_x + lattice.num_cells_x) % lattice.num_cells_x;
        by = (by % lattice.num_cells_y + lattice.num_cells_y) % lattice.num_cells_y;

        int cell_index = by * lattice.num_cells_x + bx;
        
        // Adds the agent to the lattice if cell not full - needs to be atomic
        int current_count = lattice.cell_counts[cell_index];
        if (current_count < MAX_AGENTS_PER_CELL) {
            int slot_index = (cell_index * MAX_AGENTS_PER_CELL) + current_count;
            lattice.cell_agent_indices[slot_index] = i; // We save the index of the agent inside the swarm
            lattice.cell_counts[cell_index]++;
        } else {
            // TODO: THROW AN ERROR
        }
    }
}