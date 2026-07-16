#pragma once
#include <random>
#include "GPU/structures/FoodLatticeData.h"
#include "GPU/structures/Swarm.h"

namespace FoodLatticeSystem {
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_int_distribution<int> disX(0, NUM_CELLE_X - 1);
    thread_local std::uniform_int_distribution<int> disY(0, NUM_CELLE_Y - 1);

    // When first building the sym, pass CONSTANT_FOOD_AMOUNT
    void grow(FoodLatticeData& foodLattice, float growthAmount);
}