#pragma once
#include "GPU/structures/Swarm.h"
#include "GPU/structures/SpatialLatticeData.h"

namespace SensorySystem {
    void update(SwarmData& swarm, const SpatialLatticeData& lattice);
}