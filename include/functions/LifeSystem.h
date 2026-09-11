#pragma once
#include "structures/Swarm.h"
#include "structures/GraveyardData.h"

namespace LifeSystem {
    extern thread_local std::mt19937 gen;
    extern thread_local std::uniform_int_distribution<int> disX;
    extern thread_local std::uniform_int_distribution<int> disY;

    void initSwarm(SwarmData& swarm, int num_prey, int num_predators);
    void handleDeaths(SwarmData& swarm, GraveyardData& graveyard);
    void handleBirths(SwarmData& swarm, float mutationRate, float mutationStrength);
    float getFitness(SwarmData& swarm, int index);
}