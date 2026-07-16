#pragma once
#include "GPU/structures/Swarm.h"
#include "GPU/structures/GraveyardData.h"

namespace LifeSystem {
    extern thread_local std::mt19937 gen;
    extern thread_local std::uniform_int_distribution<int> disX;
    extern thread_local std::uniform_int_distribution<int> disY;

    void initSwarm(SwarmData& swarm, int num_prey, int num_predators);
    void handleDeaths(SwarmData& swarm, GraveyardData& graveyard);
    void handleBirths(SwarmData& swarm, int mutationRate, int mutationStrength);
    float getFitness(SwarmData& swarm, int index);
}