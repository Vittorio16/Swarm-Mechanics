#pragma once
#include "GPU/structures/Swarm.h"

namespace LifeSystem {
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_int_distribution<int> disX(0, NUM_CELLE_X - 1);
    thread_local std::uniform_int_distribution<int> disY(0, NUM_CELLE_Y - 1);

    void initSwarm(SwarmData& swarm, int num_prey, int num_predators);
    void handleDeaths(SwarmData& swarm);
    void handleBirths(SwarmData& swarm);
}