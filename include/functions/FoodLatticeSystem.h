#pragma once
#include <random>
#include "structures/FoodLatticeData.h"
#include "structures/Swarm.h"

namespace FoodLatticeSystem {
    void initRNG(FoodLatticeData& foodLattice);
    // When first building the sym, pass CONSTANT_FOOD_AMOUNT
    void grow(FoodLatticeData& foodLattice, float growthAmount);
}