#pragma once
#include "GPU/structures/Swarm.h"
#include "GPU/structures/FoodLatticeData.h"

namespace EnergySystem {
    void update(SwarmData& swarm, FoodLatticeData& foodLattice, float dt);
}