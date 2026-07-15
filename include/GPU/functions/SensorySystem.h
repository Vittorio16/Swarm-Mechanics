#pragma once
#include "GPU/structures/Swarm.h"
#include "GPU/structures/SpatialLatticeData.h"
#include "GPU/structures/FoodLatticeData.h"

namespace SensorySystem {
    void update(SwarmData& swarm, const SpatialLatticeData& lattice, const FoodLatticeData& foodLattice);
}