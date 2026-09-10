#pragma once
#include "structures/Swarm.h"
#include "structures/SpatialLatticeData.h"
#include "structures/FoodLatticeData.h"

namespace SensorySystem {
    void update(SwarmData& swarm, const SpatialLatticeData& lattice, const FoodLatticeData& foodLattice, int active_agents);
}