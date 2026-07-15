#pragma once
#include <atomic>
#include "Core/Config.h"

using namespace std;

struct SpatialLatticeData{
    int num_cells_x;
    int num_cells_y;
    int total_cells;

    // Number of agents per cell
    vector<int> cell_counts;
    // Array containing the indexes of the agents in the swarm
    vector<int> cell_agent_indices;

    SpatialLatticeData(int cx, int cy) :
        num_cells_x(cx),
        num_cells_y(cy),
        total_cells(cx * cy),
        cell_counts(cx * cy, 0),
        cell_agent_indices(cx * cy * MAX_AGENTS_PER_CELL, -1) {}
};