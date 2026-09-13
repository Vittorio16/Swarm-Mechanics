#pragma once
#include <random>
#include "structures/FoodLatticeData.h"
#include "structures/Swarm.h"

namespace FoodLatticeSystem {
    // When first building the sym, pass CONSTANT_FOOD_AMOUNT
    void grow(FoodLatticeData& foodLattice, float growthAmount, uint64_t seed, const uint64_t* tick);
    void buildChunkSummary(FoodLatticeData& foodLattice);
}