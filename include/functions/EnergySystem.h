#pragma once
#include "structures/Swarm.h"
#include "structures/FoodLatticeData.h"

namespace EnergySystem {
    void update(SwarmData& swarm, FoodLatticeData& foodLattice, float dt);
}