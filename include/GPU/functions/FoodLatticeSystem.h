#pragma once
#include <random>
#include "GPU/structures/FoodLatticeData.h"
#include "GPU/structures/Swarm.h"

namespace FoodLatticeSystem {
    extern thread_local std::mt19937 gen;
    extern thread_local std::uniform_int_distribution<int> disX;
    extern thread_local std::uniform_int_distribution<int> disY;

    // When first building the sym, pass CONSTANT_FOOD_AMOUNT
    void grow(FoodLatticeData& foodLattice, float growthAmount);
}